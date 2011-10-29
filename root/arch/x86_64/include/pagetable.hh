/// @file   pagetable.hh
/// @brief  64bit paging table ops.
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_PAGETABLE_HH_
#define ARCH_X86_64_INCLUDE_PAGETABLE_HH_

#include "basic_types.hh"
#include "arch.hh"


class kernel_log;

class page_table_ent
{
	/// Page table entry type (PML4, PDPTE, PDE, PTE)
	typedef u64 type;
	type e;

public:
	enum flags {
		/// Page exist if set.
		P    = U64(1) << 0,

		/// Writable if set.
		RW   = U64(1) << 1,

		/// User table if set.
		US   = U64(1) << 2,

		/// Write through if set.
		PWT  = U64(1) << 3,

		/// Cash Disable if set.
		PCD  = U64(1) << 4,

		/// Accessed. Set by CPU.
		A    = U64(1) << 5,

		/// Dirtied. Set by CPU.
		D    = U64(1) << 6,

		/// Page size 1GiB or 2MiB if set, else 4KiB.
		PS   = U64(1) << 7,  

		///< Page attribute table enable if set (PTE).
		//PAT  = U64(1) << 7,

		///< Global page if set.
		G    = U64(1) << 8,

		/// Page attribute table enable if set (PDPTE,PDE).
		//PAT  = 1L << 12,

		/// Execute disable if set.
		/// このフラグが使えるのは IA32_EFER.NXE = 1 のときだけ。
		XD   = U64(1) << 63,
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
		e = (e & U64(0xffffff0000000fff)) | a;
	}
	void set_addr(type a) { set_adr(a); }
	type get_adr() const {
		return e & U64(0x000000fffffff000);
	}
	type get_addr() const { return get_adr(); }
	void set_avail1(u32 a) {
		e = (e & U64(0xfffffffffff1ffff)) | (a << 9);
	}
	u32 get_avail1() const {
		return static_cast<u32>((e & U64(0x00000000000e0000)) >> 9);
	}
	void set_avail2(u32 a) {
		e = (e & U64(0x800fffffffffffff)) | (static_cast<u64>(a) << 52);
	}
	u32 get_avail2() const {
		return static_cast<u32>((e & U64(0x7ff0000000000000)) >> 52);
	}
};


namespace arch {

typedef page_table_ent pte;

class page_table
{
public:
	page_table(pte* _top=0) : top(_top) {}

	cause::stype set(u64 adr, page::TYPE pt, u64 flags);
	cause::stype set_page(u64 vadr, u64 padr, page::TYPE pt, u64 flags);

	pte* get_table() { return top; }

	void dump(kernel_log& x);

public:
	enum PAGE_FLAGS {
		EXIST = pte::P,
		WRITE = pte::RW,
	};

private:
	pte* top;
};

}  // End of namespace arch.


#endif  // include guard
