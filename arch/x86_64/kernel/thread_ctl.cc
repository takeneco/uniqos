// @file   thread_ctl.cc

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

#include <native_thread.hh>

#include <global_vars.hh>
#include <mempool.hh>
#include <native_cpu_node.hh>
#include <native_ops.hh>
#include <new_ops.hh>

#include <log.hh>


namespace {

const uptr STACK_SIZE = 0x1000;

}


namespace x86 {

// native_thread class

native_thread::native_thread(
    uptr text,
    uptr param,
    uptr stack,
    uptr stack_size
) :
	thread(),
	rs(text, param, stack, stack_size)
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
	mempool* stack_mp;
};

cause::t thread_ctl::setup()
{
	auto r = mempool_acquire_shared(sizeof (native_thread), &thread_mp);
	if (is_fail(r))
		return r;

	r = mempool_acquire_shared(STACK_SIZE, &stack_mp);
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t thread_ctl::create_boot_thread()
{
	native_cpu_node* cn = get_native_cpu_node();
	native_thread* t = new (thread_mp->alloc()) native_thread(0, 0, 0, 0);

	cn->get_thread_queue().attach_boot_thread(t);

	cn->set_running_thread(t);

	return cause::OK;
}

cause::pair<native_thread*> thread_ctl::create_thread(
    cpu_node* owner_cpu,
    uptr text,
    uptr param)
{
	void* stack = stack_mp->alloc();
	if (!stack)
		return cause::pair<native_thread*>(cause::NOMEM, 0);

	void* p = thread_mp->alloc();
	if (!p) {
		stack_mp->dealloc(stack);
		return cause::pair<native_thread*>(cause::NOMEM, 0);
	}

	native_thread* t = new (p) native_thread(
	    text, param, reinterpret_cast<uptr>(stack), STACK_SIZE);

	owner_cpu->get_thread_queue().attach(t);

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

