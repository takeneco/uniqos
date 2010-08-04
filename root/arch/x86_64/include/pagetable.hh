// @file   arch/x86_64/include/pagetable.hh
// @author Kato Takeshi
// @brief  64bit paging table ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_INCLUDE_PAGETABLE_HH_
#define _ARCH_X86_64_INCLUDE_PAGETABLE_HH_

#include "btypes.hh"


class page_table_ent
{
	/// Page table entry type (PML4, PDPTE, PDE, PTE)
	typedef u64 type;
	type e;

public:
	enum flags {
		P    = 1L << 0,  ///< Page exist if set.
		RW   = 1L << 1,  ///< Writable if set.
		US   = 1L << 2,  ///< User table if set.
		PWT  = 1L << 3,  ///< Write through if set.
		PCD  = 1L << 4,  ///< Cash Disable if set.
		A    = 1L << 5,  ///< Accessed. Set by CPU.
		D    = 1L << 6,  ///< Dirtied. Set by CPU.
		PS   = 1L << 7,  ///< Page size 1GiB or 2MiB if set, else 4KiB.
		//PAT  = 1L << 7,  ///< Page attribute table enable if set (PTE).
		G    = 1L << 8,  ///< Global page if set.
		//PAT  = 1L << 12, ///< Page attribute table enable if set (PDPTE,PDE).

		/// Execute disable if set.
		/// このフラグが使えるのは IA32_EFER.NXE = 1 のときだけ。
		XD   = 1L << 63,
	};

	void set(type a, type f) {
		//e = (a & 0x000000fffffff000) | f;
		e = a | f;
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
	type test_flags(type f) const {
		return e & f;
	}
	void set_addr(type a) {
		e = (e & 0xffffff0000000fff) | a;
	}
	type get_addr() const {
		return e & 0x000000fffffff000;
	}
};

namespace arch {

typedef page_table_ent pte;

}  // End of namespace arch.

#endif  // Include guard.
