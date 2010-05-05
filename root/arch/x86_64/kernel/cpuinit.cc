// @file   arch/x86_64/kernel/cpuinit.cc
// @author Kato Takeshi
// @brief  Initialize GDT/IDT.
//
// (C) 2010 Kato Takeshi.

#include "kerninit.hh"

#include "btypes.hh"
#include "native.hh"


namespace {

class gdte {
	typedef u64 type;
	type e;

public:
	enum {
		XR  = U64CAST(0xa) << 40, ///< Exec and read.
		S   = U64CAST(1) << 44,  ///< System seg if set, Data seg if clear.
		                         ///< Always set in long mode.
		P   = U64CAST(1) << 47,  ///< Descriptor exist if set.
		AVL = U64CAST(1) << 52,  ///< Software useable.
		L   = U64CAST(1) << 53,  ///< Long mode if set.
		D   = U64CAST(1) << 54,  ///< If long mode, must be clear.
		G   = U64CAST(1) << 55,  ///< Limit scale 4096 times if set.
	};

	gdte() {}
	void set_null() {
		e = 0;
	}
	void set(type base, type limit, type dpl, type flags) {
		e = (base  & 0x00ffffff) << 16 |
		    (base  & 0xff000000) << 32 |
		    (limit & 0x0000ffff)       |
		    (limit & 0x000f0000) << 32 |
		    (dpl & 3) << 45 |
		    flags;
	}
	type get_raw() const { return e; }
};

gdte gdt[3];

cause::stype pic_init()
{
	enum {
		PIC0_IMR  = 0x0021,
		PIC0_ICW1 = 0x0020,
		PIC0_ICW2 = 0x0021,
		PIC0_ICW3 = 0x0021,
		PIC0_ICW4 = 0x0021,
		PIC1_IMR  = 0x00a1,
		PIC1_ICW1 = 0x00a0,
		PIC1_ICW2 = 0x00a1,
		PIC1_ICW3 = 0x00a1,
		PIC1_ICW4 = 0x00a1,
	};
	// Disable interrupt
	native_outb(0xff, PIC0_IMR);
	native_outb(0xff, PIC1_IMR);

	// Edge trigger mode
	native_outb(0x11, PIC0_ICW1);
	// Map IRQ 0-7 to INT 20h-27h
	native_outb(0x20, PIC0_ICW2);
	// PIC1 cascading to PIC0-IRQ2
	native_outb(1 << 2, PIC0_ICW3);
	// No buffering
	native_outb(0x01, PIC0_ICW4);

	// Edge trigger mode
	native_outb(0x11, PIC1_ICW1);
	// Map IRQ 8-15 to INT 28h-2Fh
	native_outb(0x28, PIC1_ICW2);
	// PIC1 cascading to PIC0-IRQ2
	native_outb(2, PIC1_ICW3);
	// No buffering
	native_outb(0x01, PIC1_ICW4);

	// Enable interrupt
	// PIC0 - 0x01 : Timer
	// PIC0 - 0x02 : Keyboard
	// PIC0 - 0x04 : PIC1 cascades(IRQ2)
	// PIC0 - 0x08 : COM2
	// PIC0 - 0x10 : COM1
	native_outb(0xef, PIC0_IMR);
	native_outb(0xff, PIC1_IMR);

	return cause::OK;
}

}  // End of anonymous namespace


int cpu_init()
{
	gdt[0].set_null();
	gdt[1].set(0, 0xfffff, 0,
		gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);
	gdt[2].set(0, 0xfffff, 3,
		gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);

	gdt_ptr64 gdtptr;
	gdtptr.init(sizeof gdt, (u64)gdt);
	native_lgdt(&gdtptr);

	intr_init();

	pic_init();

	return 0;
}
