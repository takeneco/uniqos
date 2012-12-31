/// @file  hpet.cc
/// @brief HPET timer.
//
// (C) 2012 KATO Takeshi
//

#include <arch.hh>
#include <clock_src.hh>
#include <config.h>
#include <log.hh>
#include <mempool.hh>
#include "mpspec.hh"
#include <native_ops.hh>
#include <new_ops.hh>

#if CONFIG_ACPI
#  include <acpi_ctl.hh>
#endif  // CONFIG_ACPI


namespace {

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
		const u64 time = counter + usecs_to_cnt(usecs);
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

private:
	cause::type on_clock_source_get_tick(tick_time* tick);

private:
	hpet_regs* const regs;

	u16  minimum_clock_tick_in_periodic;
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
	operations* _ops = new (mem_alloc(sizeof (operations))) operations;
	if (!_ops)
		return cause::NOMEM;

	_ops->init();

	_ops->get_tick = clock_source::call_on_clock_source_get_tick<hpet>;

	clock_source::ops = _ops;

	regs->disable();
	regs->counter = 0;
	regs->set_periodic_timer(TIMER_0, 1000000 /* 1sec */);
	regs->enable_legrep();
}

cause::type hpet::on_clock_source_get_tick(tick_time* tick)
{
	*tick = regs->counter;

	return cause::OK;
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

