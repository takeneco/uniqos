/// @file   cpu_idte.hh
/// @brief  IDT ops.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_CPU_IDTE_HH_
#define ARCH_X86_64_INCLUDE_CPU_IDTE_HH_

#include "basic_types.hh"


namespace arch {

class idte
{
	typedef u64 type;
	type e[2];

public:
	enum FLAGS {
		INTR = 0x0e0000000000,
		TRAP = 0x0f0000000000,
	};
	void set(type offset, type seg, type ist, type dpl, type flags) {
		e[0] = (offset & 0x000000000000ffff)       |
		       (offset & 0x00000000ffff0000) << 32 |
		       (seg    & 0x000000000000ffff) << 16 |
		       (ist    & 0x0000000000000007) << 32 |
		       (dpl    & 0x0000000000000003) << 45 |
		       (flags  & 0x00001f0000000000)       |
		       (         0x0000800000000000); // Enable(P) flag.
		e[1] = (offset & 0xffffffff00000000) >> 32;
	}
	void set(void (*handler)(), type seg, type ist, type dpl, type flags) {
		set(reinterpret_cast<type>(handler), seg, ist, dpl, flags);
	}
	void disable() {
		e[0] = e[1] = 0;
	}

	u64 get(int i) const { return e[i]; }
};

}  // namespace arch


#endif  // include guard
