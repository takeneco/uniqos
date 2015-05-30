/// @file   page.cc
/// @brief  Memory page alloc/free.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
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

#include <core/page.hh>

#include <core/cpu_node.hh>


cause::t page_alloc(arch::page::TYPE page_type, uptr* padr)
{
	const cpu_id cpuid = arch::get_cpu_node_id();

	return page_alloc(cpuid, page_type, padr);
}

cause::t page_alloc(cpu_id cpuid, arch::page::TYPE page_type, uptr* padr)
{
	cpu_node* cpu = get_cpu_node(cpuid);

	return cpu->page_alloc(page_type, padr);
}

cause::t page_dealloc(arch::page::TYPE page_type, uptr padr)
{
	cpu_node* cpu = get_cpu_node();

	return cpu->page_dealloc(page_type, padr);
}

