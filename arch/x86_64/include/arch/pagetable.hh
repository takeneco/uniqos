/// @file  arch/pagetbl.hh
/// @brief Page table interfaces.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
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

#ifndef ARCH_PAGETBL_HH_
#define ARCH_PAGETBL_HH_

#include <core/basic.hh>
#include <arch.hh>


namespace arch {
namespace page {

struct page_table;

/// Page type
enum LEVEL : s8 {
	INVALID = -1,

	L1 = 0,
	L2,
	L3,
	L4,
	L5,
	L6,
	L7,
	HIGHEST = L5, // L6, L7 are not using in the page management.

	PHYS_L1 = L1,
	PHYS_L2 = L3,
	PHYS_L3 = L5,
	PHYS_L4 = L7,
	PHYS_HIGHEST = PHYS_L4,
};
using TYPE = LEVEL;

enum {
	LEVEL_COUNT = HIGHEST + 1,
	PHYS_LEVEL_CNT = 4,
};

/// Page size
enum {
	PHYS_L1_SIZE_BITS = 12,
	PHYS_L2_SIZE_BITS = 21,
	PHYS_L3_SIZE_BITS = 30,
	PHYS_L4_SIZE_BITS = 39,

	PHYS_L1_SIZE      = U64(1) << PHYS_L1_SIZE_BITS,
	PHYS_L2_SIZE      = U64(1) << PHYS_L2_SIZE_BITS,
	PHYS_L3_SIZE      = U64(1) << PHYS_L3_SIZE_BITS,
	PHYS_L4_SIZE      = U64(1) << PHYS_L4_SIZE_BITS,

	L1_SIZE_BITS = PHYS_L1_SIZE_BITS,     // 12
	L2_SIZE_BITS = PHYS_L1_SIZE_BITS + 6, // 18
	L3_SIZE_BITS = PHYS_L2_SIZE_BITS,     // 21
	L4_SIZE_BITS = PHYS_L2_SIZE_BITS + 6, // 27
	L5_SIZE_BITS = PHYS_L3_SIZE_BITS,     // 30

	L1_SIZE      = U64(1) << L1_SIZE_BITS, // 4KiB
	L2_SIZE      = U64(1) << L2_SIZE_BITS, // 256KiB
	L3_SIZE      = U64(1) << L3_SIZE_BITS, // 2MiB
	L4_SIZE      = U64(1) << L4_SIZE_BITS, // 128MiB
	L5_SIZE      = U64(1) << L5_SIZE_BITS, // 1GiB
};

using page_flags = u16;
enum PAGE_FLAGS : page_flags
{
	DISABLE       = 1 << 0,  // bit reversed
	READ_ONLY     = 1 << 1,  // bit reversed
	DENY_USER     = 1 << 2,  // bit reversed
	WRITE_THROUGH = 1 << 3,
	CACHE_DISABLE = 1 << 4,

	ACCESSED      = 1 << 5,
	DIRTIED       = 1 << 6,

	GLOBAL        = 1 << 8,
};

int bits_of_level(unsigned int page_type);

inline uptr size_of_type(LEVEL pl)
{
	switch (pl) {
	case L1: return L1_SIZE;
	case L2: return L2_SIZE;
	case L3: return L3_SIZE;
	case L4: return L4_SIZE;
	case L5: return L5_SIZE;
	default: return 0;
	}
}

inline LEVEL type_of_size(uptr size)
{
	if (size <= L1_SIZE)      return L1;
	else if (size <= L2_SIZE) return L2;
	else if (size <= L3_SIZE) return L3;
	else if (size <= L4_SIZE) return L4;
	else if (size <= L5_SIZE) return L5;
	else                      return INVALID;
}

u64        decode_flags(page_flags flags);
page_flags encode_flags(u64 native_flags);

page_table* get_table();

cause::t map(
    page_table* tbl, uptr vadr, uptr padr, LEVEL page_type, page_flags flags);

cause::t unmap(
    page_table* tbl, uptr vadr, LEVEL page_type);

void clear_tlb(void* vadr);

}  // namespace page
}  // namespace arch


#endif  // include guard

