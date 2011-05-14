/// @file  ioapic.hh
//
// (C) 2011 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_IOAPIC_HH_
#define ARCH_X86_64_INCLUDE_IOAPIC_HH_

#include "base_types.hh"


namespace arch {

void* ioapic_base();

class ioapic_control
{
	struct memmapped_regs {
		u32 volatile ioregsel;
		u32          unused[3];
		u32 volatile iowin;
	};
	memmapped_regs* regs;

public:
	enum {
		FIXED_DERIV  = 0x00000000,
		LOWPRI_DERIV = 0x00000100,
		SMI_DERIV    = 0x00000200,
		NMI_DERIV    = 0x00000400,
		INIT_DERIV   = 0x00000500,
		EXTINT_DERIV = 0x00000700,

		LOGICAL_DEST = 0x00000800,
		LEVEL_SENS   = 0x00008000,
		LOW_ACTIVE   = 0x00002000,
	};

	ioapic_control() {}
	ioapic_control(void* base) 
	    : regs(reinterpret_cast<memmapped_regs*>(base))
	{}
	cause::stype detect() {
		regs = reinterpret_cast<memmapped_regs*>(ioapic_base());
	}

	u32 read(u32 sel) {
		regs->ioregsel = sel;
		return regs->iowin;
	}
	void write(u32 sel, u32 data) {
		regs->ioregsel = sel;
		regs->iowin = data;
	}
	void mask(u32 irq) {
		write(0x10 + irq * 2, 0x00010000);
	}
	void unmask(u32 irq, u8 cpuid, u8 vec, u32 flags=FIXED_DERIV) {
		// edge trigger, physical destination, fixd delivery
		write(0x10 + irq * 2 + 1,
		    (static_cast<u32>(cpuid) << 24) | flags);
		write(0x10 + irq * 2, vec);
	}
};

}  // namespace arch


#endif  // include guard

