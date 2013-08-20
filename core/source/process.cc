/// @file   process.cc
/// @brief  process class implementation.

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <process.hh>

#include <cpu_node.hh>
#include <native_ops.hh>
#include <string.hh>


process::process() :
	ptbl(0)
{
}

process::~process()
{
}

cause::t process::init()
{
	uptr top_padr;
	//TODO:ret val
	get_cpu_node()->page_alloc(arch::page::PHYS_L1, &top_padr);

	u8* top = static_cast<u8*>(
	    arch::map_phys_adr(top_padr, arch::page::PHYS_L1_SIZE));

	//TODO:arch depends
	u8* cr3 = reinterpret_cast<u8*>(native::get_cr3());

	//TODO:arch depends
	mem_copy(2048, &cr3[2048], &top[2048]);

	ptbl.set_toptable(top);

	return cause::OK;
}

// process::page_table_acquire

cause::pair<uptr> process::page_table_acquire::acquire(pagetable* /*x*/)
{
	cause::pair<uptr> r;
	r.r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &r.value);
	return r;
}

cause::t process::page_table_acquire::release(pagetable* /*x*/, u64 padr)
{
	return get_cpu_node()->page_dealloc(arch::page::PHYS_L1, padr);
}

