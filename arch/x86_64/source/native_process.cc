/// @file   native_process.cc

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "native_process.hh"

#include "native_thread.hh"
#include <core/cpu_node.hh>
#include <util/string.hh>
#include <x86/native_ops.hh>

#include <pagetable.hh>
#include <core/log.hh>

namespace x86 {

native_process::native_process() :
	ptbl(0)
{
}

/// Setup as boot process.
cause::t native_process::setup_self()
{
	native_thread* self_thr = get_current_native_thread();

	cause::t r = process::setup(self_thr, 0);
	if (is_fail(r))
		return r;

	ptbl.set_toptable(arch::page::get_table());

	return cause::OK;
}

cause::t native_process::setup(thread* entry_thread, int iod_nr)
{
	cause::t r = process::setup(entry_thread, iod_nr);
	if (is_fail(r))
		return r;

	uptr my_pml4_padr;
	r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &my_pml4_padr);
	if (is_fail(r))
		return r;

	arch::pte* my_pml4 = static_cast<arch::pte*>(
	    arch::map_phys_adr(my_pml4_padr, arch::page::TABLE_SIZE));

	mem_fill(0, my_pml4, arch::page::TABLE_SIZE / 2);

	arch::pte* parent_pml4 = arch::page::get_table();

	// アドレス空間の後半をカーネル用としている。
	// ページテーブルは 4096 byte で、その後半の 2048 byte を
	// コピーすることで、カーネル用アドレス空間を共有する。
	mem_copy(arch::page::TABLE_SIZE / 2,
	         &parent_pml4[256],
	         &my_pml4[256]);

	arch::page::unget_table(parent_pml4);

	ptbl.set_toptable(my_pml4);

	return cause::OK;
}

native_process* get_current_native_process()
{
	native_thread* thr = get_current_native_thread();

	return static_cast<native_process*>(thr->get_owner_process());
}

}  // namespace x86

