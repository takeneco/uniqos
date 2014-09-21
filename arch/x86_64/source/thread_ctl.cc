// @file   thread_ctl.cc

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

#include <arch/thread_ctl.hh>

#include "native_cpu_node.hh"
#include "native_thread.hh"
#include <arch/global_vars.hh>
#include <arch/native_ops.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>


namespace {

/// 1 << THREAD_SIZE_SHIFTS は thread を含むスレッドのスタックサイズ。
/// このパラメータを変える場合はブート時に割り当てるスタックの仕様も
/// 修正が必要。
const uptr THREAD_SIZE_SHIFTS = 13;

}


namespace x86 {

// native_thread class

native_thread::native_thread(
    thread_id tid,
    uptr      text,
    uptr      param,
    uptr      stack_size
) :
	thread(tid),
	rs(text, param, reinterpret_cast<uptr>(this), stack_size),
	stack_bytes(stack_size)
{
	rs.cr3 = native::get_cr3();
}


class thread_ctl
{
	DISALLOW_COPY_AND_ASSIGN(thread_ctl);

public:
	thread_ctl();

	cause::t setup();
	cause::t create_boot_thread();

	cause::pair<native_thread*> create_thread(
	    cpu_node* owner_cpu, uptr text, uptr param);
	cause::t destroy_thread(native_thread* t);

private:
	thread_id get_next_tid();

private:
	mempool* thread_mp;

	//TODO:lock
	thread_id next_tid;
};

thread_ctl::thread_ctl() :
	next_tid(1)
{
}

cause::t thread_ctl::setup()
{
	auto mp = mempool::acquire_shared(1 << THREAD_SIZE_SHIFTS);
	if (is_fail(mp))
		return mp.cause();

	thread_mp = mp;

	return cause::OK;
}

/// @brief 実行中のスレッドに対応する thread を生成する。
//
/// boot thread に対応する thread を生成するために使う。
/// boot thread には最初は thread のインスタンスが無いが、boot thread を
/// 終了するために thread のインスタンスが必要なため。
cause::t thread_ctl::create_boot_thread()
{
	native_cpu_node* cn = get_native_cpu_node();
	native_thread* t = new (*thread_mp)
	    native_thread(get_next_tid(), 0, 0, 1 << THREAD_SIZE_SHIFTS);

	cn->attach_boot_thread(t);

	cn->load_running_thread(t);

	return cause::OK;
}

cause::pair<native_thread*> thread_ctl::create_thread(
    cpu_node* owner_cpu,
    uptr text,
    uptr param)
{
	native_thread* t = new (*thread_mp)
	    native_thread(get_next_tid(), text, param, 1 << THREAD_SIZE_SHIFTS);
	if (!t) {
		return make_pair(cause::NOMEM, t);
	}

	owner_cpu->attach_thread(t);

	return make_pair(cause::OK, t);
}

/// @param[in] t  cpu から detach() された native_thread.
cause::t thread_ctl::destroy_thread(native_thread* t)
{
	new_destroy(t, *thread_mp);

	return cause::OK;
}

thread_id thread_ctl::get_next_tid()
{
	thread_id r = next_tid;

	if (++next_tid == 0)
		next_tid = 1;

	//TODO:duplicate id check

	return r;
}

/// @brief Initialize native_thread_ctl.
/// @pre mempool_init() was successful.
cause::t thread_ctl_setup()
{
	thread_ctl* tc = new (generic_mem()) thread_ctl;
	if (!tc)
		return cause::NOMEM;

	auto r = tc->setup();
	if (is_fail(r)) {
		new_destroy(tc, generic_mem());
		return r;
	}

	global_vars::arch.thread_ctl_obj = tc;

	return cause::OK;
}

cause::t create_boot_thread()
{
	return global_vars::arch.thread_ctl_obj->create_boot_thread();
}

cause::pair<native_thread*> create_thread(
    cpu_node* owner_cpu,
    uptr text,
    uptr param)
{
	return global_vars::arch.thread_ctl_obj->
	    create_thread(owner_cpu, text, param);
}

cause::pair<native_thread*> create_thread(
    cpu_node* owner_cpu,
    thread_entry_point text,
    void* param)
{
	return create_thread(owner_cpu,
	    reinterpret_cast<uptr>(text),
	    reinterpret_cast<uptr>(param));
}

cause::t destroy_thread(native_thread* t)
{
	return global_vars::arch.thread_ctl_obj->destroy_thread(t);
}

}  // namespace x86

namespace arch {

thread* get_current_thread()
{
	thread* t;
	asm ("movq %%rsp, %0 \n"
	     "andq %1, %0"
	    : "=r"(t)
	    : "n"(~((1 << THREAD_SIZE_SHIFTS) - 1)));

	return t;
}

void sleep_current_thread()
{
	x86::get_native_cpu_node()->sleep_current_thread();
}

}  // namespace arch

