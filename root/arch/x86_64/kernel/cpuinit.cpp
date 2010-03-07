/**
 * @file    arch/x86_64/kernel/cpuinit.cpp
 * @author  Kato Takeshi
 * @brief   Initialize GDT/IDT.
 *
 * (C) Kato Takeshi 2010
 */

#include "btypes.hpp"


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
		D   = 1 << 54,
		G   = 1 << 55,  ///< Limit scale 4096 times if set.
	};

	void set(type b, type m, type f) {
		e = (b & 0x0000ffff) << 16   |
		    (b & 0x00ff0000) << 16   |
		    (b & 0xff000000) << 32   |
		    (m & 0x0000ffff)         |
		    (m & 0x000f0000) << 32   |
		    f;
	}
	void set_flags(type f) {
		e |= f;
	}
	void clr_flags(type f) {
		e &= ~f;
	}
	void set_base(type b) {
		e = (e & 0x00ffff000000ffff) |
		    (b & 0x0000ffff) << 16   |
		    (b & 0x00ff0000) << 16   |
		    (b & 0xff000000) << 32;
	}
	void set_limit(type m) {
		e = (e & 0xfff0ffffffff0000) |
		    (m & 0x0000ffff)         |
		    (m & 0x000f0000) << 32;
	}
};

}  // End of anonymous namespace

extern "C" int cpu_init()
{
	return 0;
}
