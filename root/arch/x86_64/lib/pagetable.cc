/// @file   pagetable.cc
/// @brief  Page table.
//
// (C) 2011 KATO Takeshi
//

#include "pagetable.hh"

#include "log.hh"


namespace arch {

namespace {

/// [page::PHYS_L1] = 0
/// [page::PHYS_L2] = 1
/// [page::PHYS_L3] = 2
/// [page::PHYS_L4] = 3
const int PAGETYPE_TO_LEVELINDEX[] = { 0, -1, 1, -1, 2, -1, 3, };

const int PTE_INDEX_SHIFTS[] = {
	page::PHYS_L1_SIZE_BITS,
	page::PHYS_L2_SIZE_BITS,
	page::PHYS_L3_SIZE_BITS,
	page::PHYS_L4_SIZE_BITS,
};

void pte_init(pte* table)
{
	for (int i = 0; i < 512; ++i)
		table[i].set(0, 0);
}

void dump_pte(kernel_log& x, pte* table, int level);

}


cause::stype page_table::set(u64 adr, page::TYPE pt, u64 flags)
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
	     level >= target_level; --level)
	{
		const int index = (adr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;

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

	uptr p;
	const cause::stype r = page::alloc(pt, &p);
	if (r != cause::OK)
		return r;

	if (pt != page::PHYS_L1)
		flags |= pte::PS;

	const int index = (adr >> PTE_INDEX_SHIFTS[target_level]) & 0x1ff;
	table[index].set(p, flags);

	return cause::OK;
}

/// @brief  ページテーブルに物理アドレスを割り当てる。
/// @param[in] vadr  virtual address.
/// @param[in] padr  physical page address.
/// @param[in] pt    page type. one of PHYS_L*.
/// @param[in] flags page flags.
cause::stype page_table::set_page(u64 vadr, u64 padr, page::TYPE pt, u64 flags)
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

void page_table::dump(kernel_log& x)
{
	dump_pte(x, top, 4);
}

namespace {

void indents(kernel_log& x, int n)
{
	for (int i = 0; i < n; ++i)
		x("  ");
}

void dump_pte(kernel_log& x, pte* table, int depth)
{
	--depth;

	for (int i = 0; i < 512; ++i) {
		const pte& e = table[i];
		if (e.get() & pte::P) {
			indents(x, 3 - depth);
			x("[").u(u32(i), 16)("] : ").u(e.get(), 16)();
			if (!(e.get() & pte::PS) && depth >= 0) {
				pte* _table =
				    reinterpret_cast<pte*>(
				    static_cast<uptr>(e.get_adr()));
				dump_pte(x, _table, depth);
			}
		}
	}
}

}

}

