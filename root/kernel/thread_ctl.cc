// @file   thread_ctl.cc
// @brief  thread_ctl class implements.
//
// (C) 2012 KATO Takeshi
//

#include <thread_ctl.hh>

#include <mempool.hh>
#include <placement_new.hh>

#include <log.hh>


namespace {

const uptr STACK_SIZE = 0x1000;

}


thread_ctl::thread_ctl() :
	thread_mempool(0),
	stack_mempool(0)
{
}

cause::stype thread_ctl::init()
{
	mempool* mp = mempool_create_shared(sizeof (thread));
	if (!mp)
		return cause::NO_MEMORY;
	thread_mempool = mp;

	mp = mempool_create_shared(STACK_SIZE);
	if (!mp)
		return cause::NO_MEMORY;
	stack_mempool = mp;

	thread* current = static_cast<thread*>(thread_mempool->alloc());
	running_thread = current;

	return cause::OK;
}

#include <native_ops.hh>
cause::stype thread_ctl::create_thread(
    uptr text, uptr param, thread** newthread)
{
	void* stack = stack_mempool->alloc();
	if (!stack)
		return cause::NO_MEMORY;

	void* p = thread_mempool->alloc();
	if (!p) {
		stack_mempool->free(stack);
		return cause::NO_MEMORY;
	}

	thread* t = new (p) thread(
	    text, param, reinterpret_cast<uptr>(stack), STACK_SIZE);

	t->state = thread::SLEEPING;
	sleeping_queue.insert_tail(t);

	*newthread = t;

	return cause::OK;
}

cause::stype thread_ctl::wakeup(thread* t)
{
	if (t->state == thread::RUNNING)
		return cause::OK;

	sleeping_queue.remove(t);
	ready_queue.insert_tail(t);

	t->state = thread::RUNNING;

	return cause::OK;
}

extern "C" void switch_regset(arch::regset* r1, arch::regset* r2);

void thread_ctl::manual_switch()
{
	thread* prev_run = running_thread;

	thread* next_run = ready_queue.remove_head();
	if (!next_run)
		return;

	ready_queue.insert_tail(running_thread);

	running_thread = next_run;

	switch_regset(next_run->ref_regset(), prev_run->ref_regset());
}

