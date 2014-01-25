/// @file   native_cpu_node.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2013-2014 KATO Takeshi
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

#include <flags.hh>
#include <global_vars.hh>
#include <mempool.hh>
#include <native_ops.hh>
#include <native_thread.hh>


extern char on_syscall[];
extern "C" void native_switch_regset(arch::regset* r1, arch::regset* r2);

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

cause::t native_cpu_node::attach_boot_thread(thread* t)
{
	return threads.attach_boot_thread(t);
}

cause::t native_cpu_node::start_message_loop()
{
	auto r = x86::create_thread(
	    this, &message_loop_entry, this);
	if (is_fail(r))
		return r.r;
	message_thread = r.value;

	ready_thread(message_thread);

	return cause::OK;
}

/// running thread が変わったらこの関数を呼び出す必要がある
/// @param[in] t  Ptr to new running thread.
void native_cpu_node::load_running_thread(thread* t)
{
	native_thread* nt = static_cast<native_thread*>(t);

	syscall_buf.thread_private_info = nt->thread_private_info;
	intr_buf.running_thread_regset = nt->ref_regset();
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
	// syscall から swapgs で syscall_buf へアクセスできるようにする
	const uptr gs_base = reinterpret_cast<uptr>(&syscall_buf);
	native::write_msr(gs_base, 0xc0000102);

	// STAR
	const u64 msr_star =
	    (static_cast<u64>(gdt.kern_code_offset()) << 32) |
	    (static_cast<u64>(gdt.user_data_offset() + 3 - 8) << 48);
	native::write_msr(msr_star, 0xc0000081);

	// LSTAR
	native::write_msr(reinterpret_cast<u64>(on_syscall), 0xc0000082);

	// FMASK
	native::write_msr(
	    x86::REGFLAGS::IOPL | x86::REGFLAGS::IF, 0xc0000084);

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
	istf->regs = &this->intr_buf.running_thread_regset;

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

void native_cpu_node::switch_messenger_after_intr()
{
	switch_thread_after_intr(message_thread);
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void native_cpu_node::post_intr_message(message* ev)
{
	intr_msgq.push(ev);
	ready_thread_np(message_thread);

	switch_messenger_after_intr();
}

void native_cpu_node::post_soft_message(message* ev)
{
	preempt_disable();

	soft_msgq.push(ev);
	ready_thread_np(message_thread);

	preempt_enable();
}

/// @brief  Make running thread sleep.
void native_cpu_node::sleep_current_thread()
{
	arch::intr_disable();

	_sleep_current_thread();

	arch::intr_enable();
}

/// @brief  呼び出し元スレッドは READY のままでスレッドを切り替える。
/// @retval true スレッド切り替えの後、実行順が戻ってきた。
/// @retval false 切り替えるスレッドがなかった。
/// @note preempt_enable / intr_disable の状態で呼び出す必要がある。
bool native_cpu_node::force_switch_thread()
{
	thread* prev_thr = threads.get_running_thread();

	thread* next_thr = threads.switch_next_thread();

	if (!next_thr)
		return false;

	this->load_running_thread(next_thr);

	native_switch_regset(
	    static_cast<x86::native_thread*>(next_thr)->ref_regset(),
	    static_cast<x86::native_thread*>(prev_thr)->ref_regset());

	return true;
}

/// @brief  Switch thread after interrupt returned.
//
/// 割込み処理中にこの関数を使うと、割込み終了後の iret の後に実行するスレッド
/// を指定できる。割込み処理以外の状態でこの関数を呼んではならない。
void native_cpu_node::switch_thread_after_intr(native_thread* t)
{
	force_set_running_thread(t);

	load_running_thread(t);
}

/// @brief Exit boot thread.
//
/// Must to be call from boot thread.
void native_cpu_node::exit_boot_thread()
{
	arch::intr_disable();

	thread* boot_thr = threads.get_running_thread();

	thread* next_thr = threads.exit_thread(boot_thr);

	load_running_thread(next_thr);


	// 下で使っているので boot_thr はまだ開放できない

	native_switch_regset(
	    static_cast<x86::native_thread*>(next_thr)->ref_regset(),
	    static_cast<x86::native_thread*>(boot_thr)->ref_regset());
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

		bool r = force_switch_thread();

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

/// Make running thread sleep without lock.
void native_cpu_node::_sleep_current_thread()
{
	thread* prev_run = threads.get_running_thread();

	thread* next_run = threads.sleep_current_thread_np();

	// thread の anti_sleep フラグがセットされると next_run は null になる。
	if (next_run) {
		load_running_thread(next_run);

		native_switch_regset(
		    static_cast<x86::native_thread*>(next_run)->ref_regset(),
		    static_cast<x86::native_thread*>(prev_run)->ref_regset());
	}
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

}  // namespace arch

