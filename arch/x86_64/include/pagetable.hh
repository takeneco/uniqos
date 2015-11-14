/// @file   pagetable.hh
/// @brief  64bit paging table ops.

//  Uniqos  --  Unique Operating System
//  (C) 2010-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ARCH_X86_64_INCLUDE_PAGETABLE_HH_
#define ARCH_X86_64_INCLUDE_PAGETABLE_HH_

#include <arch/pagetbl.hh>
#include <core/basic.hh>


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

	void set(type padr, type flags) {
		//e = (a & 0x000000fffffff000) | f;
		e = padr | flags;
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
	void set_adr(type padr) {
		e = (e & U64(0xffffff0000000fff)) | padr;
	}
	type get_adr() const {
		return e & U64(0x000000fffffff000);
	}
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
	static const int        PAGETYPE_TO_LEVELINDEX[];
	static const page::TYPE LEVELINDEX_TO_PAGETYPE[];
	static const int        PTE_INDEX_SHIFTS[];

public:
	page_table_base(pte* _top) : top(_top) {}

	void set_toptable(void* _top) { top = static_cast<pte*>(_top); }

	pte* get_toptable() { return top; }

	void dump(log_target& x) {
		dump_pte(x, top, 4);
	}

protected:
	pte* top;
};


void pte_init(pte* table);

/// @brief Page table controller.
/// @tparam page_table_traits  Page table traits class.
//
/// page_table_traits class must provide following interfaces.
/// class page_table_traits {
/// public:
///     static cause::pair<u64> acquire_page(page_table<>* x);
///     static cause::t         release_page(page_table<>* x, u64 padr);
///
///     // Convert physical adr to virtual adr.
///     static void* phys_to_virt(u64 padr);
///     static u64 virt_to_phys(void* vadr);
/// };
template <class page_table_traits>
class page_table_tmpl : public page_table_base
{
public:
	page_table_tmpl(pte* top);
	page_table_tmpl() {}

	cause::pair<pte*> declare_table(u64 vadr, page::TYPE pt);

	cause::t set_page(u64 vadr, u64 padr, page::TYPE pt, u64 flags);

	struct page_enum {
		uptr cur_vadr;
		uptr end_vadr;
	};
	cause::t unset_page_start(
	    uptr vadr1, uptr vadr2, page_enum* pe);
	cause::t unset_page_next(
	    page_enum* pe, uptr* vadr, u64* padr, page::TYPE* pt);
	cause::t unset_page_end(
	    page_enum* pe);

private:
	pte* get_pte(pte* ent) {
		return static_cast<pte*>(
		    page_table_traits::phys_to_virt(ent->get_adr()));
	}
};

/// @param[in] top  exist page table. null available.
template <class page_table_traits>
page_table_tmpl<page_table_traits>::page_table_tmpl(pte* top) :
	page_table_base(top)
{
}

/// @brief  ページテーブルを指定した深さまで作る。アドレスは割り当てず、
///         エントリは空のままにする。
/// @param[in] vadr  virtual address.
/// @param[in] pt    page type. one of PHYS_L* without PHYS_L1.
/// @return 作成したページテーブルを返す。
///         すでにページテーブルがある場合はそのテーブルを返す。
template <class page_table_traits>
cause::pair<pte*> page_table_tmpl<page_table_traits>::declare_table(
    u64 vadr, page::TYPE pt)
{
	int target_level = PAGETYPE_TO_LEVELINDEX[pt];

	if (UNLIKELY(!top)) {
		const auto r = page_table_traits::acquire_page(this);
		if (is_fail(r))
			return cause::make_pair<pte*>(r.r, 0);

		top = static_cast<pte*>(
		    page_table_traits::phys_to_virt(r));
		pte_init(top);
	}

	pte* table = top;
	for (int level = PAGETYPE_TO_LEVELINDEX[page::PHYS_HIGHEST];
	     level > target_level;
	     --level)
	{
		const int index = (vadr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;

		if (table[index].test_flags(pte::P) == 0) {
			const auto r = page_table_traits::acquire_page(this);
			if (is_fail(r))
				return cause::make_pair<pte*>(r.r, 0);

			pte_init(static_cast<pte*>(
			    page_table_traits::phys_to_virt(r)));
			table[index].set(r.value(),
			    pte::P | pte::RW | pte::US | pte::A);
		}

		table = static_cast<pte*>(
		    page_table_traits::phys_to_virt(table[index].get_adr()));
	}

	return cause::make_pair(cause::OK, table);
}

/// @brief  ページテーブルに物理アドレスを割り当てる。
/// @param[in] vadr  virtual address.
/// @param[in] padr  physical page address.
/// @param[in] pt    page type. one of PHYS_L*.
/// @param[in] flags page flags.
template <class page_table_traits>
cause::t page_table_tmpl<page_table_traits>::set_page(
    u64 vadr, u64 padr, page::TYPE pt, u64 flags)
{
	auto r = declare_table(vadr, pt);
	if (is_fail(r))
		return r.r;

	pte* table = r.value();

	if (pt != page::PHYS_L1)
		flags |= pte::PS;

	const int target_level = PAGETYPE_TO_LEVELINDEX[pt];

	const int index = (vadr >> PTE_INDEX_SHIFTS[target_level]) & 0x1ff;
	table[index].set(padr, flags);

	return cause::OK;
}

template <class page_table_traits>
cause::t page_table_tmpl<page_table_traits>::unset_page_start(
    uptr start_vadr, uptr end_vadr, page_enum* upe)
{
	upe->cur_vadr = down_align<uptr>(start_vadr, page::PHYS_L1_SIZE);
	upe->end_vadr = down_align<uptr>(end_vadr,   page::PHYS_L1_SIZE);

	return cause::OK;
}

template <class page_table_traits>
cause::t page_table_tmpl<page_table_traits>::unset_page_next(
    page_enum* upe, uptr* vadr, u64* padr, page::TYPE* pt)
{
	pte* table = top;
	uptr cur_vadr = upe->cur_vadr;

	pte* table_stack[page::PHYS_LEVEL_CNT];

	int level = page::PHYS_LEVEL_CNT - 1;
	for (;;) {
		int index = (cur_vadr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;
		while (index < 512) {
			if (cur_vadr > upe->end_vadr)
				return cause::END;

			pte* ent = &table[index];
			if (ent->test_flags(pte::P)) {
				if (ent->test_flags(pte::PS) || level == 0) {
					*vadr = cur_vadr;
					*padr = ent->get_adr();
					*pt = LEVELINDEX_TO_PAGETYPE[level];
					upe->cur_vadr = cur_vadr +
					   (UPTR(1) << PTE_INDEX_SHIFTS[level]);
					ent->set(0, 0);
					return cause::OK;
				}
				else {
					table_stack[level] = table;
					--level;
					table = static_cast<pte*>(
					    page_table_traits::phys_to_virt(
					    ent->get_adr()));
					break;
				}
			}
			else {
				++index;
				cur_vadr += UPTR(1) << PTE_INDEX_SHIFTS[level];
				continue;
			}
		}
		if (index >= 512) {
			int i;
			for (i = 0; i < 512; ++i) {
				if (table[i].test_flags(pte::P))
					break;
			}

			if (i == 512) {
				u64 padr =
				    page_table_traits::virt_to_phys(table);
				auto r =
				    page_table_traits::release_page(this, padr);
				if (is_fail(r))
					return r;
			}

			++level;
			if (level >= page::PHYS_LEVEL_CNT)
				return cause::END;

			table = table_stack[level];
		}
	}
}

template <class page_table_traits>
cause::t page_table_tmpl<page_table_traits>::unset_page_end(page_enum* pe)
{
	pe->cur_vadr = UPTR(-1);

	return cause::OK;
}


}  // namespace arch


#endif  // include guard

