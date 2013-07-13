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
	cause::t r = setup_tss();
	if (is_fail(r))
		return r;

	r = setup_gdt();
	if (is_fail(r))
		return r;

	r = global_vars::arch.cpu_ctl_common_obj->setup_idt();
	if (is_fail(r))
		return r;

	return cause::OK;
}

void cpu_ctl::set_running_thread(thread* t)
{
	running_thread_regset = t->ref_regset();
}

cause::t cpu_ctl::setup_tss()
{
	tss.iomap_base = sizeof tss;

	mempool* ist_mp;
	cause::t r = mempool_acquire_shared(IST_BYTES, &ist_mp);
	if (is_fail(r))
		return r;

	void* ist_intr = ist_mp->alloc();
	void* ist_trap = ist_mp->alloc();
	mempool_release_shared(ist_mp);
	if (!ist_intr || !ist_trap)
		return cause::NOMEM;

	tss.set_ist(ist_layout(ist_intr), IST_INTR);
	tss.set_ist(ist_layout(ist_trap), IST_TRAP);

	syscall_buf.running_thread_regset_p = &running_thread_regset;
	// syscall から swapgs で syscall_buf へアクセスできるようにする
	const uptr gs_base = reinterpret_cast<uptr>(&syscall_buf);
	native::write_msr(gs_base, 0xc0000100);

	// STAR
	const u64 msr_star =
	    (static_cast<u64>(gdt.kern_code_offset()) << 32) |
	    (static_cast<u64>(gdt.user_data_offset() + 3 - 8) << 48);
	native::write_msr(msr_star, 0xc0000081);

	// LSTAR
	native::write_msr(reinterpret_cast<u64>(on_syscall), 0xc0000082);

	// FMASK
	native::write_msr(0x00003000L, 0xc0000084);

	return cause::OK;
}

cause::t cpu_ctl::setup_gdt()
{
	gdt.null_entry.set_null();
	gdt.kern_code.set(0);
	gdt.kern_data.set(0);
	gdt.user_code.set(3);
	gdt.user_data.set(3);
	gdt.tss.set(&tss, sizeof tss, 0);

	native::gdt_ptr64 gdtptr;
	gdtptr.set(sizeof gdt, &gdt);
	native::lgdt(&gdtptr);

	native::set_ds(GDT::kern_data_offset());
	native::set_es(GDT::kern_data_offset());
	native::set_fs(GDT::kern_data_offset());
	native::set_gs(GDT::kern_data_offset());
	native::set_ss(GDT::kern_data_offset());

	native::ltr(GDT::tss_offset());

	return cause::OK;
}

/// @brief IST へ ist_footer_layout を書く。
/// @return IST として使用するポインタを返す。
//
/// 割り込みハンドラが IST から情報をたどれるようにする。
void* cpu_ctl::ist_layout(void* mem)
{
	void* memf = static_cast<u8*>(mem) + IST_BYTES;

	ist_footer_layout* istf = static_cast<ist_footer_layout*>(memf);

	istf -= 1;

	istf->proc = static_cast<cpu_node*>(this);
	istf->regs = &this->running_thread_regset;

	return istf;
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

cause::type cpu_setup()
{
	return get_cpu_node()->setup();
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

