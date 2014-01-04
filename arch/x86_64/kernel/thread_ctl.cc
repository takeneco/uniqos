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

#include <native_thread.hh>
#include <global_vars.hh>
#include <mempool.hh>
#include <native_cpu_node.hh>
#include <native_ops.hh>
#include <new_ops.hh>

#include <log.hh>


namespace {

/// 1 << THREAD_SIZE_SHIFTS は thread を含むスレッドのスタックサイズ。
/// このパラメータを変える場合はブート時に割り当てるスタックの仕様も
/// 修正が必要。
const uptr THREAD_SIZE_SHIFTS = 13;

}


namespace x86 {

// native_thread class

native_thread::native_thread(
    uptr text,
    uptr param,
    uptr stack_size
) :
	thread(),
	rs(text, param, reinterpret_cast<uptr>(this), stack_size),
	stack_bytes(stack_size)
{
	rs.cr3 = native::get_cr3();
}


class thread_ctl
{
	DISALLOW_COPY_AND_ASSIGN(thread_ctl);

public:
	thread_ctl() {}

	cause::t setup();
	cause::t create_boot_thread();

	cause::pair<native_thread*> create_thread(
	    cpu_node* owner_cpu, uptr text, uptr param);

private:
	mempool* thread_mp;
};

cause::t thread_ctl::setup()
{
	auto r = mempool_acquire_shared(1 << THREAD_SIZE_SHIFTS, &thread_mp);
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t thread_ctl::create_boot_thread()
{
	native_cpu_node* cn = get_native_cpu_node();
	native_thread* t = new (thread_mp->alloc()) native_thread(0, 0, 0);

	cn->attach_boot_thread(t);

	cn->load_running_thread(t);

	return cause::OK;
}

cause::pair<native_thread*> thread_ctl::create_thread(
    cpu_node* owner_cpu,
    uptr text,
    uptr param)
{
	void* p = thread_mp->alloc();
	if (!p) {
		return cause::pair<native_thread*>(cause::NOMEM, 0);
	}

	native_thread* t = new (p) native_thread(
	    text, param, 1 << THREAD_SIZE_SHIFTS);

	owner_cpu->attach_thread(t);

	return cause::pair<native_thread*>(cause::OK, t);
}

cause::t thread_ctl_setup()
{
	void* p = mem_alloc(sizeof (thread_ctl));

	thread_ctl* tc = new (mem_alloc(sizeof (thread_ctl))) thread_ctl;
	if (!tc)
		return cause::NOMEM;

	auto r = tc->setup();
	if (is_fail(r)) {
		tc->~thread_ctl();
		mem_dealloc(p);
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
    cpu_node* owner_cpu, uptr text, uptr param)
{
	return global_vars::arch.thread_ctl_obj->
	    create_thread(owner_cpu, text, param);
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
