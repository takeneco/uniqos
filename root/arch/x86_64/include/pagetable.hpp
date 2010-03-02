/* @file    arch/x86_64/include/pagetable.hpp
 * @version 0.0.0.1
 * @author  Kato Takeshi
 * @brief   64bit paging table ops.
 */
// (C) Kato.T 2010

#ifndef _ARCH_X86_64_INCLUDE_PAGETABLE_HPP
#define _ARCH_X86_64_INCLUDE_PAGETABLE_HPP

#include "btypes.hpp"

namespace arch {

class pte
{
	/// Page table entry type (PML4, PDPTE, PDE, PTE)
	typedef _u64 type;
	type e;

	enum flags {
		P    = 1 << 0,  ///< Page exist if set.
		RW   = 1 << 1,  ///< Writable if set.
		US   = 1 << 2,  ///< User table if set.
		PWT  = 1 << 3,  ///< Write through if set.
		PCD  = 1 << 4,  ///< Cash Disable if set.
		A    = 1 << 5,  ///< Access if set.
		D    = 1 << 6,  ///< Dirty if set.
		PS   = 1 << 7,  ///< Page size 2MiB if set, else 4KiB.
		//PAT  = 1 << 7,  ///< Page attrbute table enable if set.
		G    = 1 << 8,  ///< Global page if set.
	};

public:
	void set(type a, type f) {
		e = (a 0x000000fffffff000) | f;
	}
	type get() {
		return e;
	}
	void set_flags(type f) {
		e |= f;
	}
	void clr_flags(type f) {
		e &= ~f;
	}
	void set_addr(type a) {
		e = (e & 0xffffff0000000fff) | a;
	}
	type get_addr() {
		return e & 0x000000fffffff000;
	}
};

}  // End of namespace arch.

#endif  // Include guard.
