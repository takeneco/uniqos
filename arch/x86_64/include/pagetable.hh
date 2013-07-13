/// @file   pagetable.hh
/// @brief  64bit paging table ops.
//
// (C) 2010-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_PAGETABLE_HH_
#define ARCH_X86_64_INCLUDE_PAGETABLE_HH_

#include <basic_types.hh>
#include <arch.hh>


class log_target;

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


void dump_pte(log_target& x, pte* table, int level);

class page_table_base
{
public:
	enum PAGE_FLAGS {
		EXIST = pte::P,
		WRITE = pte::RW,
	};

protected:
	static const int PAGETYPE_TO_LEVELINDEX[];
	static const int PTE_INDEX_SHIFTS[];

public:
	page_table_base(pte* _top) : top(_top) {}

	void set_toptable(void* _top) { top = static_cast<pte*>(_top); }

	pte* get_table() { return top; }

	void dump(log_target& x) {
		dump_pte(x, top, 4);
	}

protected:
	pte* top;
};


void pte_init(pte* table);

/// @brief Page table creater.
/// @tparam page_table_acquire Memory allocator class, must having
///                    following public function.
///  - static cause::pair<u64>
///    page_table_acquire::acquire(page_table<>* x)
///  - static cause::t
///    page_table_acquire::release(page_table<>* x, u64 padr)
/// @tparam p2v        physical to virtual convert function.
template <class page_table_acquire, void* (*p2v)(u64 padr)>
class page_table : public page_table_base
{
public:
	page_table(pte* top);
	page_table() {}

	cause::t set_page(u64 vadr, u64 padr, page::TYPE pt, u64 flags);
};

/// @param[in] top  exist page table. null available.
template <class page_table_acquire, void* (*p2v)(u64 padr)>
page_table<page_table_acquire, p2v>::page_table(pte* top)
    : page_table_base(top)
{
}

/// @brief  ページテーブルに物理アドレスを割り当てる。
/// @param[in] vadr  virtual address.
/// @param[in] padr  physical page address.
/// @param[in] pt    page type. one of PHYS_L*.
/// @param[in] flags page flags.
template <class page_table_acquire, void* (*p2v)(u64 padr)>
cause::t page_table<page_table_acquire, p2v>::set_page(
    u64 vadr, u64 padr, page::TYPE pt, u64 flags)
{
	const int target_level = PAGETYPE_TO_LEVELINDEX[pt];

	if (UNLIKELY(!top)) {
		const auto r = page_table_acquire::acquire(this);
		if (r.is_fail())
			return r.r;

		top = static_cast<pte*>(p2v(r.value));
		pte_init(top);
	}

	pte* table = top;
	for (int level = PAGETYPE_TO_LEVELINDEX[page::PHYS_HIGHEST];
	     level > target_level;
	     --level)
	{
		const int index = (vadr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;

		if (table[index].test_flags(pte::P) == 0) {
			const auto r = page_table_acquire::acquire(this);
			if (r.is_fail())
				return r.r;

			pte_init(static_cast<pte*>(p2v(r.value)));
			table[index].set(r.value,
			    pte::P | pte::RW | pte::US | pte::A);
		}

		table = static_cast<pte*>(p2v(table[index].get_adr()));
	}

	if (pt != page::PHYS_L1)
		flags |= pte::PS;

	const int index = (vadr >> PTE_INDEX_SHIFTS[target_level]) & 0x1ff;
	table[index].set(padr, flags);

	return cause::OK;
}

}  // namespace arch


#endif  // include guard
