/// @file   pagetable.hh
/// @brief  64bit paging table ops.
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_PAGETABLE_HH_
#define ARCH_X86_64_INCLUDE_PAGETABLE_HH_

#include "basic_types.hh"
#include "arch.hh"


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
	type get() const {
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
	void set_adr(type a) {
		e = (e & 0xffffff0000000fff) | a;
	}
	void set_addr(type a) { set_adr(a); }
	type get_adr() const {
		return e & 0x000000fffffff000;
	}
	type get_addr() const { return get_adr(); }
	void set_avail1(u32 a) {
		e = (e & 0xfffffffffff1ffff) | (a << 9);
	}
	u32 get_avail1() const {
		return static_cast<u32>((e & 0x00000000000e0000) >> 9);
	}
	void set_avail2(u32 a) {
		e = (e & 0x800fffffffffffff) | (static_cast<u64>(a) << 52);
	}
	u32 get_avail2() const {
		return static_cast<u32>((e & 0x7ff0000000000000) >> 52);
	}
};


namespace arch {

typedef page_table_ent pte;

class page_table
{
public:
	page_table() : top(0) {}

	cause::stype set(uptr adr, page::TYPE pt, uptr flags);

public:
	enum PAGE_FLAGS {
		WRITE = pte::RW,
	};

private:
	pte* top;
};

}  // End of namespace arch.


#endif  // include guard
