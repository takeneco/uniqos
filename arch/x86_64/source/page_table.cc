// @file   page_table.cc

//  Uniqos  --  Unique Operating System
//  (C) 2014-2015 KATO Takeshi
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

#include "page_table.hh"

#include <core/cpu_node.hh>


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

