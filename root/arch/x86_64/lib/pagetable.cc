/// @file   pagetable.cc
/// @brief  Page table.
//
// (C) 2011 KATO Takeshi
//

#include "pagetable.hh"


namespace {

/// [page::PHYS_L1] = 0
/// [page::PHYS_L2] = 1
/// [page::PHYS_L3] = 2
/// [page::PHYS_L4] = 3
const int PAGETYPE_TO_LEVELINDEX = { 0, -1, 1, -1, 2, -1, 3, };

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

}


namespace arch {

cause::stype page_table::set(uptr adr, page::TYPE pt, uptr flags)
{
	const int taget_level = PAGETYPE_TO_LEVELINDEX[pt];

	if (UNLIKELY(!top)) {
		uptr page_adr;
		const cause::stype r = page::alloc(page::PHYS_L1, &page_adr);
		if (r != cause::OK)
			return r;

		top = reinterpret_cast<pte*>(page_adr);
	}

	pte* table = top;
	for (int level = PAGETYPE_TO_LEVELINDEX[page::PHYS_HIGHEST];
	     level > target_level; --level)
	{
		const int index = (adr >> PTE_INDEX_SHIFTS[level]) & 0x1ff;

		if (table[index].test_flags(pte::P) == 0) {
			uptr p;
			const cause::stype r = page::alloc(page::PHYS_L1, &p);
			if (r != cause::OK)
				return r;

			pte_init(reinterpret_cast<pte*>(p));
			table[index].set(p, pte::P | pte::RW | pte::PS);
		}

		table = reinterpret_cast<pte*>(table[index].get_adr());
	}

	uptr p;
	const cause::stype r = page::alloc(pt, &p);
	if (r != cause::OK)
		return r;

	const int index = (adr >> PTE_INDEX_SHIFTS[target_level]) & 0x1ff;
	table[index].set(p, flags);

	return cause::OK;
}

}

