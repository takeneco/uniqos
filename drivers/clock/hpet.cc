/// @file  hpet.cc
/// @brief HPET timer driver.
//
// (C) 2012-2013 KATO Takeshi
//

#include <arch.hh>
#include <clock_src.hh>
#include <config.h>
#include <cpu_node.hh>
#include <global_vars.hh>
#include <intr_ctl.hh>
#include <irq_ctl.hh>
#include <log.hh>
#include <mempool.hh>
#include <message.hh>
#include "mpspec.hh"
#include <native_ops.hh>
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

struct hpet_regs
{
	u32          caps_l32;
	u32          caps_h32;
	u64          reserved1;
	u64 volatile configs;
	u64          reserved2;
	u64 volatile intr_status;
	u64          reserved3[25];
	u64 volatile counter;
	u64          reserved4;

	struct timer_regs
	{
		u32 volatile config_l32;
		u32 volatile config_h32;
		u64 volatile comparator;
		u64 volatile fsb_intr;
		u64          reserved;
	};
	timer_regs timer[32];

	void enable_legrep() { configs |= 0x3; }
	void disable() { configs &= ~0x3; }

	u64 usecs_to_cnt(u64 usecs) const {
		return usecs * U64(1000000000) / caps_h32;
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
		timer[i].config_l32 =
		    (timer[i].config_l32 & 0xffff8030) |
		    0x00000004 |
		    (intr_route << 9);
	}

	/// @brief  enable periodic timer
	/// 周期タイマを使うときはこの関数だけでは設定できない。
	/// 初期化手順から合わせる必要がある。
	void set_periodic_timer(
	    timer_index i, u64 interval_usecs, u32 intr_route=0)
	{
		timer[i].config_l32 =
		    (timer[i].config_l32 & 0xffff8030) |
		    0x0000004c |
		    (intr_route << 9);
		timer[i].comparator = usecs_to_cnt(interval_usecs);
	}

	void unset_timer(timer_index i) {
		timer[i].config_l32 = timer[i].config_l32 & 0xffff8030;
		timer[i].comparator = U64(0xffffffffffffffff);
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

	cause::type setup();
	void handler1();

private:
	cause::type setup_ops();
	cause::type setup_intr();
	u64 get_clock();

	cause::pair<tick_time> on_clock_source_UpdateClock();
	cause::type on_clock_source_SetTimer(tick_time clock, message* msg);
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
cause::type hpet_detect_acpi(hpet** dev)
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

cause::type hpet::setup()
{
	cause::type r = setup_ops();
	if (is_fail(r))
		return r;

	r = setup_intr();
	if (is_fail(r))
		return r;

	clock_period = regs->caps_h32;

	regs->disable();
	regs->counter = 0;
	regs->set_periodic_timer(TIMER_0, 1000000 /* 1sec */);
	regs->enable_legrep();

	regs->set_nonperiodic_timer(TIMER_1, 0);

	return cause::OK;
}

namespace {

u64 t;
bool msg_posted;
void _msg(message*)
{
	msg_posted=false;
	log(2)("t=").u(t)('\n');
	log()("t=").u(t)('\n');
	//log()("t=").u(t)();
	++t;
}

message msg;
void _handler0(intr_handler* h)
{
	if (msg_posted)
		return;

	msg_posted = true;
	msg.handler = _msg;

	cpu_node* cpu = get_cpu_node();
	cpu->post_intr_message(&msg);

	cpu->switch_messenger_after_intr();
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
		cpu_node* cpu = get_cpu_node();
		cpu->post_intr_message(msg1);
		cpu->switch_messenger_after_intr();
		msg1 = 0;
	}
}

cause::type hpet::setup_intr()
{
t=0;
msg_posted=false;
	auto h = new (mem_alloc(sizeof (intr_handler_with<hpet*>)))
	    intr_handler_with<hpet*>(_handler0, this);
	u32 vec;
	arch::irq_interrupt_map(2, &vec);

	global_vars::core.intr_ctl_obj->install_handler(vec, h);


	h = new (mem_alloc(sizeof (intr_handler_with<hpet*>)))
	    intr_handler_with<hpet*>(_handler1, this);
	arch::irq_interrupt_map(8, &vec);

	global_vars::core.intr_ctl_obj->install_handler(vec, h);

	return cause::OK;
}

cause::type hpet::setup_ops()
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

cause::type hpet::on_clock_source_SetTimer(tick_time clock, message* msg)
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

cause::type hpet_detect(hpet** dev)
{
	cause::type r;

#if CONFIG_ACPI
	r = hpet_detect_acpi(dev);
	if (is_ok(r))
		return r;

#endif  // CONFIG_ACPI

	return cause::FAIL;
}

}  // namespace

cause::type hpet_setup(clock_source** clksrc)
{
	hpet* h;
	cause::type r = hpet_detect(&h);
	if (is_fail(r))
		return r;

	h->setup();

	if (CONFIG_DEBUG_VERBOSE > 0)
		log()("hpet:")(h)();

	*clksrc = h;

	return cause::OK;
}

