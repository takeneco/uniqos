/// @file  apic.cc
/// @brief Control APIC.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "kerninit.hh"
#include "native_ops.hh"

#include "output.hh"


namespace {

/// Local APIC Registers
enum {
	LOCAL_APIC_REG_BASE = arch::PHYSICAL_MEMMAP_BASEADR + 0xfee00000,

	/// ID
	LOCAL_APIC_ID = 0x0020,
	/// Version
	LOCAL_APIC_VERSION = 0x0030,
	/// Task Priority Register
	LOCAL_APIC_TPR = 0x0080,
	/// ?
	LOCAL_APIC_APR = 0x0090,
	/// Processor Priority Register
	LOCAL_APIC_PPR = 0x00a0,
	/// End of Interrupt
	LOCAL_APIC_EOI = 0x00b0,
	/// Logical Destination Register
	LOCAL_APIC_LDR = 0x00d0,
	/// Destination Format Register
	LOCAL_APIC_DFR = 0x00e0,
	/// Spurious Interrupt Vector Register
	LOCAL_APIC_SVR = 0x00f0,
	/// In Service Register
	LOCAL_APIC_ISR_BASE = 0x0100,
	/// Trigger Mode Register
	LOCAL_APIC_TMR_BASE = 0x0180,
	/// Interrupt Request Register
	LOCAL_APIC_IRR_BASE = 0x0200,
	/// Error Status Register
	LOCAL_APIC_ERROR    = 0x0280,
	/// Interrupt Command Register (0-31)
	LOCAL_APIC_ICR_LOW  = 0x0300,
	/// Interrupt Command Register (32-63)
	LOCAL_APIC_ICR_HIGH = 0x0310,
	/// LVT Timer Register
	LOCAL_APIC_LVT_TIMER = 0x0320,
	/// Initial Count Register (timer)
	LOCAL_APIC_INI_COUNT = 0x0380,
	/// Current Count Register (timer)
	LOCAL_APIC_CUR_COUNT = 0x0390,
	/// Divide Configuration Register (timer)
	LOCAL_APIC_DCR = 0x03e0,
};

inline u32* local_apic_reg(uptr offset) {
	return reinterpret_cast<u32*>(LOCAL_APIC_REG_BASE + offset);
}

cause::stype local_apic_init()
{
	volatile u32* reg;

	kern_get_out()->put_str("local apic version = ")->
	    put_u64hex(*local_apic_reg(LOCAL_APIC_VERSION))->put_endl();
/*
	// Local APIC enable
	reg = local_apic_reg(LOCAL_APIC_SVR);
	// 32bit アクセスは必須。
	// コンパイラの最適化で 64bit アクセスにならないように注意。
	u32 tmp = *reg;
	// *reg = tmp | 0x0100;
	asm volatile ("movl %1, %0" : "=m" (*reg) : "r" (tmp | 0x100));
*/
	*local_apic_reg(LOCAL_APIC_ID) = 0;
	*local_apic_reg(LOCAL_APIC_LDR) = 0x1000000;
	*local_apic_reg(LOCAL_APIC_DFR) = 0xffffffff;

	// Task priority lowest
	*local_apic_reg(LOCAL_APIC_TPR) = 0;

	//*local_apic_reg(LOCAL_APIC_DCR) = 0xb; // clock/1
	*local_apic_reg(LOCAL_APIC_DCR) = 0xa; // clock/128

	// one shot, unmask, とりあえずベクタ0x30
	*local_apic_reg(LOCAL_APIC_LVT_TIMER) = arch::INTR_APIC_TIMER;

	/*
	reg = local_apic_reg(LOCAL_APIC_INI_COUNT);
	for (;;) {
		*reg = 0x800000;
		asm volatile("hlt");
		kern_get_out()->put_c('x');
	}
	*/

	/*
	int d;
	asm volatile (
	"cpuid" : "=d"(d): "a"(1)
	);
	kern_get_out()->put_str("cpuid(1)=")->put_u32hex(d)->put_c('\n');
	*/

	return cause::OK;
}

}

namespace arch {

cause::stype apic_init()
{
	return local_apic_init();
}

void wait(u32 n)
{
	volatile u32* reg = local_apic_reg(LOCAL_APIC_INI_COUNT);
	*reg = n;
	native::hlt();
}

void lapic_eoi()
{
	*local_apic_reg(LOCAL_APIC_EOI) = 0;
}

}

void lapic_dump()
{
	kern_output* kout = kern_get_out();
	kout->put_str("isr:");

	u32* reg = local_apic_reg(LOCAL_APIC_ISR_BASE);
	reg += 4 * 8;
	for (int i = 0; i < 8; ++i) {
		reg -= 4;
		u32 r = *reg;
		kout->put_u32hex(r);
		kout->put_c(' ');
	}

	kout->put_endl();
}

extern "C" void interrupt_timer()
{
	//kern_get_out()->put_c('.');

	*local_apic_reg(LOCAL_APIC_EOI) = 0;
}


