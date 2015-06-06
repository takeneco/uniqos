// @file   x86_64/source/pagetbl.cc
// @brief  Page table control interfaces.

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

#include <arch/pagetbl.hh>

#include <core/cpu_node.hh>
#include "native_pagetbl.hh"
#include <x86/native_ops.hh>


namespace arch {
namespace page {

const u64 REVERSED_FLAGS = DISABLE | READ_ONLY | DENY_USER;

page_table* get_table()
{
	return reinterpret_cast<page_table*>(native::get_cr3());
}

/// @brief page_flagsをCPUのページフラグへ変換する。
u64 decode_flags(page_flags flags)
{
	u64 native_flags =
	    ( flags & ~REVERSED_FLAGS) |
	    (~flags &  REVERSED_FLAGS);

	return native_flags;
}

/// @brief  CPUのページフラグをpage_flagsへ変換する。
page_flags encode_flags(u64 native_flags)
{
	page_flags flags = static_cast<page_flags>(
	    ( native_flags & ~REVERSED_FLAGS) |
	    (~native_flags &  REVERSED_FLAGS)
	);

	return flags;
}

/// @brief  Mapping page.
cause::t map(
    page_table* tbl,
    uptr vadr,
    uptr padr,
    LEVEL level,
    page_flags flags)
{
	x86::native_page_table pgtbl(reinterpret_cast<pte*>(tbl));

	return pgtbl.set_page(vadr, padr, level, decode_flags(flags));
}

/// @brief  Unmapping page.
cause::t unmap(
    page_table* tbl,
    uptr vadr,
    LEVEL /*level*/)
{
	x86::native_page_table pgtbl(reinterpret_cast<pte*>(tbl));

	x86::native_page_table::page_enum pe;
	pgtbl.unset_page_start(vadr, vadr, &pe);

	uptr tmp_vadr, tmp_padr;
	LEVEL tmp_pt;
	cause::t r = pgtbl.unset_page_next(&pe, &tmp_vadr, &tmp_padr, &tmp_pt);

	pgtbl.unset_page_end(&pe);

	return r;
}

/// 指定したvadrのTLBをクリアする
void clear_tlb(void* vadr)
{
	asm volatile ("invlpg %0" : : "m"(*vadr));
}

}  // namespace page
}  // namespace arch

namespace x86 {

cause::pair<uptr> page_table_traits::acquire_page(native_page_table*)
{
	cause::pair<uptr> r;
	r.r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &r.value);
	return r;
}

cause::t page_table_traits::release_page(native_page_table*, u64 padr)
{
	return get_cpu_node()->page_dealloc(arch::page::PHYS_L1, padr);
}

}  // namespace x86

