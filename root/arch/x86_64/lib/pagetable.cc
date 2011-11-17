/// @file   pagetable.cc
/// @brief  Page table.
//
// (C) 2011 KATO Takeshi
//

#include "pagetable.hh"

#include "log.hh"


namespace arch {

/// [page::PHYS_L1] = 0
/// [page::PHYS_L2] = 1
/// [page::PHYS_L3] = 2
/// [page::PHYS_L4] = 3
const int page_table_base::PAGETYPE_TO_LEVELINDEX[] = {
	0, -1, 1, -1, 2, -1, 3,
};

const int page_table_base::PTE_INDEX_SHIFTS[] = {
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


inline void indents(kernel_log& x, int n)
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

}  // namespace arch

