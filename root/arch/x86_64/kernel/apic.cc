/// @file  apic.cc
/// @brief Control APIC.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "native_ops.hh"

#include "output.hh"


namespace {

/// Local APIC Registers
enum {
	LOCAL_APIC_REG_BASE = arch::PHYSICAL_MEMMAP_BASEADR + 0xfee00000,

	/// Task Priority Register
	LOCAL_APIC_TPR = 0x0080,
	/// Spurious Interrupt Vector Register
	LOCAL_APIC_SVR = 0x00f0,
	/// LVT Timer Register
	LOCAL_APIC_LVT_TIMER = 0x0320,
	/// Initial Count Register (timer)
	LOCAL_APIC_INI_COUNT = 0x0380,
	/// Current Count Register (timer)
	LOCAL_APIC_CUR_COUNT = 0x0390,
	/// Divide Configuration Register (timer)
	LOCAL_APIC_DCR = 0x03e0,
};

inline u64* local_apic_reg(uptr offset) {
	return reinterpret_cast<u64*>(LOCAL_APIC_REG_BASE + offset);
}

cause::stype local_apic_init()
{
	volatile u64* reg;

	// Local APIC enable
	reg = local_apic_reg(LOCAL_APIC_SVR);
	*reg |= 0x0100;

/*
	// Task priority lowest
	*local_apic_reg(LOCAL_APIC_TPR) = 0;

	// Timer

	// *local_apic_reg(LOCAL_APIC_DCR) = 0xb; // clock/1
	*local_apic_reg(LOCAL_APIC_DCR) = 0xa; // clock/128
	// one shot, unmask, とりあえずベクタ0x30
	*local_apic_reg(LOCAL_APIC_LVT_TIMER) = arch::INTR_APIC_TIMER;

	*local_apic_reg(LOCAL_APIC_INI_COUNT) = 1000;

	kern_get_out()->put_u64hex(*local_apic_reg(LOCAL_APIC_CUR_COUNT));
*/
	return cause::OK;
}

}

namespace arch {

cause::stype apic_init()
{
	return local_apic_init();
}

}

extern "C" void on_timer()
{
	kern_get_out()->put_c('.');
	for (;;)
		native::hlt();
}


