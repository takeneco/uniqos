/// @file  ioapic.hh
//
// (C) 2011 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_IOAPIC_HH_
#define ARCH_X86_64_INCLUDE_IOAPIC_HH_

#include "base_types.hh"


void* ioapic_base();

class ioapic_control
{
	struct memmapped_regs {
		u32 volatile ioregsel;
		u32          unused[3];
		u32 volatile iowin;
	};
	memmapped_regs* const regs;

public:
	ioapic_control(void* base) 
	    : regs(reinterpret_cast<memmapped_regs*>(base))
	{}
	u32 read(u32 sel) {
		regs->ioregsel = sel;
		return regs->iowin;
	}
	void write(u32 sel, u32 data) {
		regs->ioregsel = sel;
		regs->iowin = data;
	}
	void mask(u32 index) {
		write(0x10 + index * 2, 0x00010000);
	}
	void unmask(u32 index, u8 cpuid, u8 vec) {
		// edge trigger, physical destination, fixd delivery
		write(0x10 + index * 2 + 1, static_cast<u32>(cpuid) << 24);
		write(0x10 + index * 2, vec);
	}
};


#endif  // include guard

