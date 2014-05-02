/// @file   cpu_ctl.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2014 KATO Takeshi
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

#include <cpu_ctl.hh>

#include <arch.hh>
#include <arch/global_vars.hh>
#include <core/cpu_node.hh>
#include <core/mempool.hh>
#include "kerninit.hh"
#include <native_ops.hh>
#include <new_ops.hh>
#include <regset.hh>


extern char on_syscall[];

namespace {

enum {
	IST_BYTES = 0x2000,  // size of ist entry
};

struct ist_footer_layout
{
	arch::regset** regs;
	cpu_node* proc;
};

}  // namespace


namespace x86 {

/**
 * native_cpu_ctl の初期化方法
 * 最初に setup() を １回だけ呼び出す。
 * その後、各CPUから load() を１回ずつ呼び出す。
 */

native_cpu_ctl::native_cpu_ctl()
{
}

cause::t native_cpu_ctl::setup()
{
	cause::t r = mps.load();
	if (is_fail(r))
		return r;

	r = idt.init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t native_cpu_ctl::load()
{
	native::idt_ptr64 idtptr;
	idtptr.set(sizeof (idte) * 256, idt.get());

	native::lidt(&idtptr);

	return cause::OK;
}


cause::t cpu_ctl_setup()
{
	native_cpu_ctl* obj =
	    new (mem_alloc(sizeof (native_cpu_ctl))) native_cpu_ctl;
	global_vars::arch.native_cpu_ctl_obj = obj;
	if (!obj)
		return cause::NOMEM;

	cause::t r = obj->setup();
	if (is_fail(r))
		return r;

	return cause::OK;
}

}  // namespace x86

namespace arch {

void intr_enable()
{
	native::sti();
}

void intr_disable()
{
	native::cli();
}

void intr_wait()
{
	asm volatile ("sti;hlt;cli" : : : "memory");
}

}  // namespace arch

