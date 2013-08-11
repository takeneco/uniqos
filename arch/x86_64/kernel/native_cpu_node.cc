/// @file   native_cpu_node.cc

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

#include <native_cpu_node.hh>

#include <global_vars.hh>
#include <mempool.hh>
#include <native_ops.hh>
#include <native_thread.hh>


extern char on_syscall[];
extern "C" void switch_regset(arch::regset* r1, arch::regset* r2);

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

// x86::native_cpu_node

native_cpu_node::native_cpu_node() :
	preempt_disable_cnt(0)
{
}

cause::t native_cpu_node::setup()
{
	cause::t r = cpu_node::setup();
	if (is_fail(r))
		return r;

	r = setup_tss();
	if (is_fail(r))
		return r;

	r = setup_gdt();
	if (is_fail(r))
		return r;

	r = global_vars::arch.cpu_ctl_common_obj->setup_idt();
	if (is_fail(r))
		return r;

	r = setup_syscall();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t native_cpu_node::start_message_loop()
{
	auto r = x86::create_thread(
	    this, &message_loop_entry, this);
	if (is_fail(r))
		return r.r;
	message_thread = r.value;

	thread_q.ready(message_thread);

	return cause::OK;
}

void native_cpu_node::set_running_thread(thread* t)
{
	running_thread_regset = static_cast<native_thread*>(t)->ref_regset();
}

cause::t native_cpu_node::setup_tss()
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

	return cause::OK;
}

cause::t native_cpu_node::setup_syscall()
{
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

cause::t native_cpu_node::setup_gdt()
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
void* native_cpu_node::ist_layout(void* mem)
{
	void* memf = static_cast<u8*>(mem) + IST_BYTES;

	ist_footer_layout* istf = static_cast<ist_footer_layout*>(memf);

	istf -= 1;

	istf->proc = static_cast<cpu_node*>(this);
	istf->regs = &this->running_thread_regset;

	return istf;
}

/// Preemption contorl

void native_cpu_node::preempt_disable()
{
#if CONFIG_PREEMPT
	arch::intr_disable();

	inc_preempt_disable();

#endif  // CONFIG_PREEMPT
}

void native_cpu_node::preempt_enable()
{
#if CONFIG_PREEMPT
	if (dec_preempt_disable() == 0)
		arch::intr_enable();

#endif  // CONFIG_PREEMPT
}

void native_cpu_node::ready_messenger()
{
	thread_q.ready(message_thread);
}

void native_cpu_node::ready_messenger_np()
{
	thread_q.ready_np(message_thread);
}

void native_cpu_node::switch_messenger_after_intr()
{
	thread_q.switch_thread_after_intr(message_thread);
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void native_cpu_node::post_intr_message(message* ev)
{
	intr_msgq.push(ev);
	thread_q.ready_np(message_thread);

	switch_messenger_after_intr();
}

void native_cpu_node::post_soft_message(message* ev)
{
	preempt_disable();

	soft_msgq.push(ev);
	thread_q.ready_np(message_thread);

	preempt_enable();
}

/// @brief  Make running thread sleep.
void native_cpu_node::sleep_current_thread()
{
	thread* prev_run = thread_q.get_running_thread();

	arch::intr_disable();

	thread* next_run = thread_q.sleep_current_thread();

	if (next_run) {
		set_running_thread(next_run);

		switch_regset(
		    static_cast<x86::native_thread*>(next_run)->ref_regset(),
		    static_cast<x86::native_thread*>(prev_run)->ref_regset());
	}

	arch::intr_enable();
}

void native_cpu_node::message_loop()
{
	preempt_disable();

	for (;;) {
		arch::intr_enable();
		arch::intr_disable();

		if (intr_msgq.deliv_all_np())
			continue;

		if (soft_msgq.deliv_np())
			continue;


		// preempt_enable / intr_disable の状態を作る
		dec_preempt_disable();

		bool r = thread_q.force_switch_thread();

		// preempt_disable / intr_disable の状態に戻す
		inc_preempt_disable();

		if (r)
			continue;

		arch::intr_wait();
	}
}

void native_cpu_node::message_loop_entry(void* _cpu_node)
{
	static_cast<native_cpu_node*>(_cpu_node)->message_loop();
}


x86::native_cpu_node* get_native_cpu_node()
{
	const cpu_id id = arch::get_cpu_id();

	return static_cast<x86::native_cpu_node*>(
	    global_vars::core.cpu_node_objs[id]);
}

x86::native_cpu_node* get_native_cpu_node(cpu_id cpuid)
{
	return static_cast<x86::native_cpu_node*>(
	    global_vars::core.cpu_node_objs[cpuid]);
}

cause::t cpu_setup()
{
	return get_native_cpu_node()->setup();
}

}  // namespace x86

void preempt_disable()
{
#if CONFIG_PREEMPT
	x86::get_native_cpu_node()->preempt_disable();

#endif  // CONFIG_PREEMPT
}

void preempt_enable()
{
#if CONFIG_PREEMPT
	x86::get_native_cpu_node()->preempt_enable();

#endif  // CONFIG_PREEMPT
}

// cpu_ctl_common
/*
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
*/
namespace arch {

void post_intr_message(message* msg)
{
	x86::get_native_cpu_node()->post_intr_message(msg);
}

void post_cpu_message(message* msg)
{
	x86::get_native_cpu_node()->post_soft_message(msg);
}

void post_cpu_message(message* msg, cpu_node* cpu)
{
	static_cast<x86::native_cpu_node*>(cpu)->post_soft_message(msg);
}

/*
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
*/
}  // namespace arch

