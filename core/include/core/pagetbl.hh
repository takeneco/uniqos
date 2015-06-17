// @file   core/pagetbl.hh
// @brief  Page table interfaces.

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

#ifndef CORE_PAGETBL_HH_
#define CORE_PAGETBL_HH_

#include <arch/pagetbl.hh>


//using page_table = arch::page::page_table;
using page_level = arch::page::LEVEL;
using page_flags = arch::page::page_flags;

enum PAGE_FLAGS {
	PAGE_DISABLE       = arch::page::DISABLE,
	PAGE_READ_ONLY     = arch::page::READ_ONLY,
	PAGE_DENY_USER     = arch::page::DENY_USER,
	PAGE_WRITE_THROUGH = arch::page::WRITE_THROUGH,
	PAGE_CACHE_DISABLE = arch::page::CACHE_DISABLE,

	PAGE_ACCESSED      = arch::page::ACCESSED,
	PAGE_DIRTIED       = arch::page::DIRTIED,

	PAGE_GLOBAL        = arch::page::GLOBAL,
};

struct PAGE_TRAITS
{
	enum FALGS {
		PHYSICAL = 1 << 0,
	};

	page_level level;
	u8         size_bits;
	u8         flags;

	bool is_physical_page() const { return (flags & PHYSICAL) != 0; }
};

struct PAGE_TRAITS_ARRAY
{
	int          traits_nr;

	/// ページサイズで昇順に並べておく必要がある。
	PAGE_TRAITS* traits_array;
};

uptr page_size_of_level(page_level level);
page_level page_level_of_size(uptr size);

cause::t page_map(
    page_table* pgtbl,
    uptr vadr,
    uptr padr,
    page_level level,
    page_flags flags);

cause::t page_unmap(
    page_table* pgtbl,
    uptr vadr,
    page_level level);


#endif  // include guard

