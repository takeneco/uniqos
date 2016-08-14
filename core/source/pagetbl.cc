/// @file   core/sources/pagetbl.cc
/// @brief  Common page table interfaces.

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

#include <core/pagetbl.hh>


uptr page_size_of_level(page_level level)
{
	PAGE_TRAITS_ARRAY pta = arch::page::get_page_traits();

	if (0 <= level && level < pta.traits_nr)
		return UPTR(1) << pta.traits_array[level].size_bits;
	else
		return 0;
}

page_level page_level_of_size(uptr size)
{
	PAGE_TRAITS_ARRAY pta = arch::page::get_page_traits();

	for (int level = 0; level < pta.traits_nr; ++level) {
		uptr page_size = UPTR(1) << pta.traits_array[level].size_bits;
		if (page_size >= size) {
			// この値は level と同じはず。
			return pta.traits_array[level].level;
		}
	}

	return static_cast<page_level>(-1);
}

/// @brief Set page table map.
//
/// @note アクティブなページテーブルを変更するときはpgtblにnullptrを
///   指定する必要がある。pgtblがnullptrのときはTLBをクリアする。
cause::t page_map(
    page_table* pgtbl,
    uptr vadr,
    uptr padr,
    page_level level,
    page_flags flags)
{
	page_table* _pgtbl = pgtbl ? pgtbl : arch::page::get_table();

	cause::t r = arch::page::map(_pgtbl, vadr, padr, level, flags);

	if (!pgtbl || (flags & PAGE_GLOBAL))
		arch::page::clear_tlb(reinterpret_cast<void*>(vadr));

	if (!pgtbl)
		arch::page::unget_table(_pgtbl);

	return r;
}

/// @brief Unset page table map.
//
/// @note アクティブなページテーブルを変更するときはpgtblにnullptrを
///   指定する必要がある。pgtblがnullptrのときはTLBをクリアする。
cause::t page_unmap(
    page_table* pgtbl,
    uptr vadr,
    page_level level)
{
	page_table* _pgtbl = pgtbl ? pgtbl : arch::page::get_table();

	cause::t r = arch::page::unmap(_pgtbl, vadr, level);

	if (!pgtbl)
		arch::page::clear_tlb(reinterpret_cast<void*>(vadr));

	if (!pgtbl)
		arch::page::unget_table(_pgtbl);

	return r;
}

