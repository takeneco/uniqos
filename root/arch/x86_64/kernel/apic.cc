/// @file  apic.cc
/// @brief Control APIC.
//
// (C) 2010 KATO Takeshi
//

#include "btypes.hh"
#include "arch.hh"


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

	// Task priority lowest
	*local_apic_reg(LOCAL_APIC_TPR) = 0;

	// Timer
	*local_apic_reg(LOCAL_APIC_DCR) = 0xb; // clock/1
	// one shot, unmask, とりあえずベクタ0x30
	*local_apic_reg(LOCAL_APIC_LVT_TIMER) = arch::INTR_APIC_TIMER;

	return cause::OK;
}

}

namespace arch {

cause::stype apic_init()
{
	return local_apic_init();
}

}

