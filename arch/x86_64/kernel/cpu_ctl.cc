/// @file   cpu_ctl.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2013 KATO Takeshi
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

#include <arch.hh>
#include <cpu_node.hh>
#include <global_vars.hh>
#include "kerninit.hh"
#include <log.hh>
#include <mempool.hh>
#include <native_ops.hh>
#include <new_ops.hh>
#include <native_thread.hh>


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


namespace arch {

// arch::cpu_ctl

cause::t cpu_ctl::setup()
{
	cause::t r = global_vars::arch.cpu_ctl_common_obj->setup_idt();
	if (is_fail(r))
		return r;

	return cause::OK;
}

}  // namespace arch

// cpu_ctl_common

cpu_ctl_common::cpu_ctl_common()
{
}

cause::t cpu_ctl_common::init()
{
	cause::t r = mps.load();
	if (is_fail(r))
		return r;

	r = idt.init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t cpu_ctl_common::setup_idt()
{
	native::idt_ptr64 idtptr;
	idtptr.set(sizeof (idte) * 256, idt.get());

	native::lidt(&idtptr);

	return cause::OK;
}


cause::t cpu_common_init()
{
	cpu_ctl_common* obj =
	    new (mem_alloc(sizeof (cpu_ctl_common))) cpu_ctl_common;
	global_vars::arch.cpu_ctl_common_obj = obj;
	if (!obj)
		return cause::NOMEM;

	cause::t r = obj->init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

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

