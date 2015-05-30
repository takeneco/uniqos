// @file   pagetable.cc
// @brief  page table control interfaces.

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

#include <arch/pagetable.hh>

#include <native_ops.hh>
#include "page_table.hh"


namespace arch {
namespace page {

const u64 REVERSED_FLAGS = PAGE_DISABLE | READ_ONLY | DENY_USER;

u64 decode_flags(pageflags flags)
{
	u64 native_flags =
	    ( flags & ~REVERSED_FLAGS) |
	    (~flags &  REVERSED_FLAGS);

	return native_flags;
}

pageflags encode_flags(u64 native_flags)
{
	pageflags flags = static_cast<pageflags>(
	    ( native_flags & ~REVERSED_FLAGS) |
	    (~native_flags &  REVERSED_FLAGS)
	);

	return flags;
}

cause::t _map(uptr vadr, uptr padr, arch::page::TYPE page_type, u64 page_flags)
{
	x86::page_table pgtbl(reinterpret_cast<pte*>(native::get_cr3()));

	return pgtbl.set_page(vadr, padr, page_type, page_flags);
}

cause::t unmap(uptr vadr, arch::page::TYPE page_type)
{
	x86::page_table pgtbl(reinterpret_cast<pte*>(native::get_cr3()));

	x86::page_table::page_enum pe;
	pgtbl.unset_page_start(vadr, vadr, &pe);

	uptr tmp_vadr, tmp_padr;
	arch::page::TYPE tmp_pt;
	cause::t r = pgtbl.unset_page_next(&pe, &tmp_vadr, &tmp_padr, &tmp_pt);

	pgtbl.unset_page_end(&pe);

	return r;
}

}  // namespace page
}  // namespace arch

