/// @file   native_process.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include "native_process.hh"

#include <core/cpu_node.hh>
#include <util/string.hh>
#include <x86/native_ops.hh>


namespace x86 {

native_process::native_process() :
	ptbl(0)
{
}

cause::t native_process::setup(thread* entry_thread, int iod_nr)
{
	cause::t r = process::setup(entry_thread, iod_nr);
	if (is_fail(r))
		return r;

	uptr top_padr;
	r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &top_padr);
	if (is_fail(r))
		return r;

	u8* top = static_cast<u8*>(
	    arch::map_phys_adr(top_padr, arch::page::PHYS_L1_SIZE));

	mem_fill(0, top, 2048);

	u8* cr3 = reinterpret_cast<u8*>(native::get_cr3());

	// アドレス空間の後半をカーネル用としている。
	// ページテーブルは 4096 byte で、その後半の 2048 byte を
	// コピーすることで、カーネル用アドレス空間を共有する。
	mem_copy(2048, &cr3[2048], &top[2048]);

	ptbl.set_toptable(top);

	return cause::OK;
}

}  // namespace x86

