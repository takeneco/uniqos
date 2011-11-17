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

namespace arch {

/// @brief Page Table Entry
class pte
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


void dump_pte(kernel_log& x, pte* table, int level);

class page_table_base
{
protected:
	pte* top;

	static const int PAGETYPE_TO_LEVELINDEX[];
	static const int PTE_INDEX_SHIFTS[];

public:
	enum PAGE_FLAGS {
		EXIST = pte::P,
		WRITE = pte::RW,
	};

public:
	page_table_base(pte* _top) : top(_top) {}

	pte* get_table() { return top; }

	void dump(kernel_log& x) {
		dump_pte(x, top, 4);
	}
};


void pte_init(pte* table);

/// @brief Page table creater.
/// @tparam page_alloc Memory allocator class, must having
///                    following public function.
///  - cause::stype page_alloc::alloc(u64* padr)
///  - cause::stype page_alloc::free(u64 padr)
template <class page_alloc>
class page_table : public page_table_base
{
	page_alloc* alloc;
public:
	page_table(pte* top, page_alloc* _alloc);

	cause::stype set_page(u64 vadr, u64 padr, page::TYPE pt, u64 flags);
};

/// @param[in] top  exist page table. null available.
template <class page_alloc>
page_table<page_alloc>::page_table(pte* top, page_alloc* _alloc)
    : page_table_base(top), alloc(_alloc)
{
}

/// @brief  ページテーブルに物理アドレスを割り当てる。
/// @param[in] vadr  virtual address.
/// @param[in] padr  physical page address.
/// @param[in] pt    page type. one of PHYS_L*.
/// @param[in] flags page flags.
template <class page_alloc>
cause::stype page_table<page_alloc>::set_page(
    u64 vadr, u64 padr, page::TYPE pt, u64 flags)
{
	const int target_level = PAGETYPE_TO_LEVELINDEX[pt];

	if (UNLIKELY(!top)) {
		uptr page_adr;
		const cause::stype r = page::alloc(page::PHYS_L1, &page_adr);
		if (r != cause::OK)
			return r;

		top = reinterpret_cast<pte*>(page_adr);
		pte_init(top);
	}

	pte* table = top;
	for (int level = PAGETYPE_TO_LEVELINDEX[page::PHYS_HIGHEST];
	     level > target_level; --level)
	{
		const int index = (vadr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;

		if (table[index].test_flags(pte::P) == 0) {
			uptr p;
			const cause::stype r = page::alloc(page::PHYS_L1, &p);
			if (r != cause::OK)
				return r;

			pte_init(reinterpret_cast<pte*>(p));
			table[index].set(p, pte::P | pte::RW);
		}

		table = reinterpret_cast<pte*>(table[index].get_adr());
	}

	if (pt != page::PHYS_L1)
		flags |= pte::PS;

	const int index = (vadr >> PTE_INDEX_SHIFTS[target_level]) & 0x1ff;
	table[index].set(padr, flags);

	return cause::OK;
}

}  // namespace arch


#endif  // include guard
