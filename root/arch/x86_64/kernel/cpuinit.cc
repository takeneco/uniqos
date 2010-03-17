// @file   arch/x86_64/kernel/cpuinit.cpp
// @author Kato Takeshi
// @brief  Initialize GDT/IDT.
//
// (C) Kato Takeshi 2010

#include "btypes.hh"


namespace {

class gdte {
	typedef _u64 type;
	type e;

public:
	enum {
		XR  = 0xa << 40, ///< Exec and read.
		S   = 1 << 44,  ///< System seg if set, Data seg if clear.
		                ///< Always set in long mode.
		P   = 1 << 47,  ///< Descriptor exist if set.
		AVL = 1 << 52,  ///< Software useable.
		L   = 1 << 53,  ///< Long mode if set.
		D   = 1 << 54,  ///< If long mode, must be clear.
		G   = 1 << 55,  ///< Limit scale 4096 times if set.
	};

	void set(type b, type m, type dpl, type f) {
		e = (b & 0x00ffffff) << 16 |
		    (b & 0xff000000) << 32 |
		    (m & 0x0000ffff)       |
		    (m & 0x000f0000) << 32 |
		    (dpl & 3) << 45 |
		    f;
	}
	gdte(type x) : e(x) {}
	gdte(type b, type m, type dpl, type f) {
		set(b, m, dpl, f);
	}
};

}  // End of anonymous namespace


extern "C" int cpu_init()
{
	static const gdte gdt[] = {
		gdte(0),
		gdte(0, 0xfffff, 0,
			gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);
		gdte(0, 0xfffff, 3,
			gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);
	};

	gidt_ptr64 gdtptr;
	gdtptr.init(sizeof gdt, gdt);

	native_lgdt(&gdtptr);

	return 0;
}
