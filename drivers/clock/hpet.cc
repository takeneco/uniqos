/// @file  hpet.cc
/// @brief HPET timer driver.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <arch.hh>
#include <clock_src.hh>
#include <config.h>
#include <core/cpu_node.hh>
#include <core/global_vars.hh>
#include <core/intr_ctl.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/message.hh>
#include <irq_ctl.hh>
#include "mpspec.hh"
#include <new_ops.hh>

#if CONFIG_ACPI
#  include <acpi_ctl.hh>
#endif  // CONFIG_ACPI


namespace {

enum
{
	DEFAULT_HPET_VEC_0 = 0x5c,
	DEFAULT_HPET_VEC_1 = 0x5d,
};

enum timer_index
{
	TIMER_0 = 0,
	TIMER_1 = 1,
};

enum {
	/// @{
	/// General Capabilities and ID Register
	GCAPSL32_REV_ID_MASK            = 0x000000ff,
	GCAPSL32_REV_ID_SHIFT           = 0,

	GCAPSL32_NUM_TIM_CAP_MASK       = 0x00001f00,
	GCAPSL32_NUM_TIM_CAP_SHIFT      = 8,

	GCAPSL32_COUNT_SIZE_CAP_MASK    = 0x00002000,
	GCAPSL32_COUNT_SIZE_CAP_SHIFT   = 13,

	GCAPSL32_LEG_ROUTE_CAP_MASK     = 0x00008000,
	GCAPSL32_LEG_ROUTE_CAP_SHIFT    = 15,

	GCAPSL32_VENDOR_ID_MASK         = 0xffff0000,
	GCAPSL32_VENDOR_ID_SHIFT        = 16,
	/// @}

	/// @{
	/// General Configuration Register
	GCONFL32_ENABLE_CNF_MASK        = 0x00000001,
	GCONFL32_ENABLE_CNF_SHIFT       = 0,

	GCONFL32_LEG_RT_CNF_MASK        = 0x00000002,
	GCONFL32_LEG_RT_CNF_SHIFT       = 1,
	/// @}

	/// @{
	/// Timer N Configuration and Capabilities Register
	TCONFL32_INT_TYPE_CNF_MASK      = 0x00000002,
	TCONFL32_INT_TYPE_CNF_SHIFT     = 1,

	TCONFL32_INT_ENB_CNF_MASK       = 0x00000004,
	TCONFL32_INT_ENB_CNF_SHIFT      = 2,

	TCONFL32_TYPE_CNF_MASK          = 0x00000008,
	TCONFL32_TYPE_CNF_SHIFT         = 3,

	TCONFL32_PER_INT_CAP_MASK       = 0x00000010,
	TCONFL32_PER_INT_CAP_SHIFT      = 4,

	TCONFL32_SIZE_CAP_MASK          = 0x00000020,
	TCONFL32_SIZE_CAP_SHIFT         = 5,

	TCONFL32_VAL_SET_CNF_MASK       = 0x00000040,
	TCONFL32_VAL_SET_CNF_SHIFT      = 6,

	TCONFL32_32MODE_CNF_MASK        = 0x00000100,
	TCONFL32_32MODE_CNF_SHIFT       = 8,

	TCONFL32_INT_ROUTE_CNF_MASK     = 0x00003e00,
	TCONFL32_INT_ROUTE_CNF_SHIFT    = 9,

	TCONFL32_FSB_EN_CNF_MASK        = 0x00004000,
	TCONFL32_FSB_EN_CNF_SHIFT       = 14,

	TCONFL32_FSB_INT_DEL_CAP_MASK   = 0x00008000,
	TCONFL32_FSB_INT_DEL_CAP_SHIFT  = 15,
	/// @}
};

struct hpet_regs
{
	u32          gcaps_l32;
	u32          gcaps_h32;
	u64          reserved1;
	u32 volatile gconf_l32;
	u32          gconf_h32; ///< reserved
	u64          reserved2;
	u64 volatile intr_status;
	u64          reserved3[25];
	u64 volatile counter;
	u64          reserved4;

	struct timer_regs
	{
		u32 volatile tconf_l32;
		u32 volatile tconf_h32;
		u64 volatile comparator;
		u64 volatile fsb_intr;
		u64          reserved;
	};
	timer_regs timer[32];

	void enable() { gconf_l32 |= 0x1; }
	void enable_legrep() { gconf_l32 |= 0x3; }
	void disable() { gconf_l32 &= ~0x3; }

	u64 usecs_to_cnt(u64 usecs) const {
		return usecs * U64(1000000000) / gcaps_h32;
	}
	u64 nanosecs_to_cnt(u64 nanosecs) const {
		return nanosecs * U64(1000000) / gcaps_h32;
	}

	/// @brief  enable nonperiodic timer.
	/// - not FSB delivery.
	/// - 64bit timer
	/// - edge trigger.
	/// @param  i  index of timer.
	/// @param  intr_route  I/O APIC input pin number.
	///   range : 0-31
	///   ignored in LegacyReplacement mode.
	void set_nonperiodic_timer(timer_index i, u32 intr_route=0)
	{
		timer[i].tconf_l32 =
		    (timer[i].tconf_l32 & 0xffff8030) |
		    0x00000004 |
		    (intr_route << 9);
	}

	/// @brief  enable periodic timer
	/// 周期タイマを使うときはこの関数だけでは設定できない。
	/// 初期化手順から合わせる必要がある。
	void set_periodic_timer(
	    timer_index i, u64 interval_usecs, u32 intr_route=0)
	{
		timer[i].tconf_l32 =
		    (timer[i].tconf_l32 & 0xffff8030) |
		    0x0000004c |
		    (intr_route << 9);
		timer[i].comparator = usecs_to_cnt(interval_usecs);
	}

	void unset_timer(timer_index i) {
		timer[i].tconf_l32 = timer[i].tconf_l32 & 0xffff8030;
	}

	/// set time on nonperiodicmode
	/// @param  usecs  specify micro secs.
	u64 set_oneshot_time(timer_index i, u64 usecs) {
		//const u64 time = counter + usecs_to_cnt(usecs);
		const u64 time = usecs;
		timer[i].comparator = time;
		return time;
	}
};


class hpet : public clock_source
{
	DISALLOW_COPY_AND_ASSIGN(hpet);
	friend class clock_source;

public:

#if CONFIG_ACPI
	hpet(const ACPI_TABLE_HPET* e);
#endif  // CONFIG_ACPI

	cause::t setup();
	void handler1();

private:
	cause::t setup_ops();
	cause::t setup_intr();
	u64 get_clock();

	cause::pair<tick_time> on_clock_source_UpdateClock();
	cause::t on_clock_source_SetTimer(tick_time clock, message* msg);
	cause::pair<u64> on_clock_source_ClockToNanosec(u64 clock);
	cause::pair<u64> on_clock_source_NanosecToClock(u64 nanosec);

private:
	hpet_regs* const regs;

	u64  clock_period;  ///< Frequency = 10^15 / clock_period
	u16  minimum_clock_tick_in_periodic;

	message* msg1;
};

#if CONFIG_ACPI

hpet::hpet(const ACPI_TABLE_HPET* e) :
	regs(static_cast<hpet_regs*>(
	    arch::map_phys_adr(e->Address.Address, sizeof (hpet_regs))))
{
}

namespace {
cause::t hpet_detect_acpi(hpet** dev)
{
	ACPI_TABLE_HPET* hpet_entry;
	ACPI_STATUS r = AcpiGetTable(ACPI_SIG_HPET, 0,
	    reinterpret_cast<ACPI_TABLE_HEADER**>(&hpet_entry));
	if (ACPI_FAILURE(r))
		return cause::FAIL;

	hpet* h = new (mem_alloc(sizeof (hpet))) hpet(hpet_entry);
	if (!h)
		return cause::NOMEM;

	*dev = h;

	return cause::OK;
}
}  // namespace

#endif  // CONFIG_ACPI

cause::t hpet::setup()
{
	cause::t r = setup_ops();
	if (is_fail(r))
		return r;

	r = setup_intr();
	if (is_fail(r))
		return r;

	clock_period = regs->gcaps_h32;

	auto _1sec_clks = nanosec_to_clock(1000000000);
	if (is_fail(_1sec_clks))
		return _1sec_clks.get_cause();

	regs->disable();
	regs->counter = 0;
	regs->set_periodic_timer(TIMER_0, 1000000 /* 1sec */, 0);
	//regs->set_periodic_timer(TIMER_0, _1sec_clks.value, 0);
	regs->enable_legrep();

	regs->set_nonperiodic_timer(TIMER_1, 2);

	log()("HPET:timer[0].tconf_h32=").x(regs->timer[0].tconf_h32)();
	log()("HPET:timer[0].tconf_l32=").x(regs->timer[0].tconf_l32)();
	log()("HPET:timer[1].tconf_h32=").x(regs->timer[1].tconf_h32)();
	log()("HPET:timer[1].tconf_l32=").x(regs->timer[1].tconf_l32)();

	log()("HPET:clock_period=").u(clock_period)();
	log()("HPET:clock/1sec=").u(_1sec_clks.value)();

	return cause::OK;
}

namespace {

u64 t;
bool msg_posted;
void _msg(message* m)
{
	msg_posted=false;
	//log(2)("t=").u(t)('\n');
	log()("t=").u(t)('\n');
	++t;

	auto m2 = static_cast<message_with<hpet*>*>(m);
}

message_with<hpet*> msg;
void _handler0(intr_handler* h)
{
	auto hh = static_cast<intr_handler_with<hpet*>*>(h);

	if (msg_posted)
		return;

	msg_posted = true;
	msg.handler = _msg;
	msg.data = hh->data;

	arch::post_intr_message(&msg);
}

void _handler1(intr_handler* h)
{
	auto hh = static_cast<intr_handler_with<hpet*>*>(h);
	hh->data->handler1();
}

}  // namespace

void hpet::handler1()
{
	if (msg1) {
		arch::post_intr_message(msg1);
		msg1 = 0;
	}
}

cause::t hpet::setup_intr()
{
t=0;
msg_posted=false;
	auto h = new (mem_alloc(sizeof (intr_handler_with<hpet*>)))
	    intr_handler_with<hpet*>(_handler0, this);
	u32 vec = 0x5e;
	arch::irq_interrupt_map(2, &vec);

	global_vars::core.intr_ctl_obj->install_handler(vec, h);


	h = new (mem_alloc(sizeof (intr_handler_with<hpet*>)))
	    intr_handler_with<hpet*>(_handler1, this);
	vec = 0x5f;
	arch::irq_interrupt_map(8, &vec);

	global_vars::core.intr_ctl_obj->install_handler(vec, h);

	return cause::OK;
}

cause::t hpet::setup_ops()
{
	operations* _ops = new (mem_alloc(sizeof (operations))) operations;
	if (!_ops)
		return cause::NOMEM;

	_ops->init();

	_ops->UpdateClock =
	    clock_source::call_on_clock_source_UpdateClock<hpet>;
	_ops->SetTimer =
	    clock_source::call_on_clock_source_SetTimer<hpet>;
	_ops->ClockToNanosec =
	    clock_source::call_on_clock_source_ClockToNanosec<hpet>;
	_ops->NanosecToClock =
	    clock_source::call_on_clock_source_NanosecToClock<hpet>;

	clock_source::ops = _ops;

	return cause::OK;
}

u64 hpet::get_clock()
{
	u64 r = regs->counter;

	LatestClock = r;

	return r;
}

cause::pair<tick_time> hpet::on_clock_source_UpdateClock()
{
	tick_time clk(get_clock());

	return cause::pair<tick_time>(cause::OK, clk);
}

cause::t hpet::on_clock_source_SetTimer(tick_time clock, message* msg)
{
	msg1 = msg;
	regs->set_oneshot_time(TIMER_1, clock);

	tick_time now(get_clock());
	if (clock < now) {
		// clock を過ぎている
		msg1 = 0;
		return cause::OUTOFRANGE;
	}

	return cause::OK;
}

cause::pair<u64> hpet::on_clock_source_ClockToNanosec(u64 clock)
{
	const u64 nanosec =
	    clock_period * clock / (1000000000000000 / 1000000000);

	return cause::pair<u64>(cause::OK, nanosec);
}

cause::pair<u64> hpet::on_clock_source_NanosecToClock(u64 nanosec)
{
	const u64 clock =
	    (1000000000000000 / 1000000000) * nanosec / (clock_period);

	return cause::pair<u64>(cause::OK, clock);
}

cause::t hpet_detect(hpet** dev)
{
	cause::t r;

#if CONFIG_ACPI
	r = hpet_detect_acpi(dev);
	if (is_ok(r))
		return r;

#endif  // CONFIG_ACPI

	return cause::FAIL;
}

}  // namespace

cause::t hpet_setup(clock_source** clksrc)
{
	hpet* h;
	cause::t r = hpet_detect(&h);
	if (is_fail(r))
		return r;

	h->setup();

	if (CONFIG_DEBUG_VERBOSE > 0)
		log()("hpet:")(h)();

	*clksrc = h;

	return cause::OK;
}

