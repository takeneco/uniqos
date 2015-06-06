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


using page_table = arch::page::page_table;
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

