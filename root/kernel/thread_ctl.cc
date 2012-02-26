// @file   thread_ctl.cc
// @brief  thread_ctl class implements.
//
// (C) 2012 KATO Takeshi
//

#include <thread_ctl.hh>

#include <global_vars.hh>
#include <mempool.hh>
#include <placement_new.hh>
#include <processor.hh>

#include <log.hh>
#include <native_ops.hh>


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
		return cause::NOMEM;
	thread_mempool = mp;

	mp = mempool_create_shared(STACK_SIZE);
	if (!mp)
		return cause::NOMEM;
	stack_mempool = mp;

	thread* current = static_cast<thread*>(thread_mempool->alloc());
	running_thread = current;

	return cause::OK;
}

cause::stype thread_ctl::create_thread(
    uptr text, uptr param, thread** newthread)
{
	void* stack = stack_mempool->alloc();
	if (!stack)
		return cause::NOMEM;

	void* p = thread_mempool->alloc();
	if (!p) {
		stack_mempool->free(stack);
		return cause::NOMEM;
	}

	thread* t = new (p) thread(&global_vars::gv.logical_cpu_obj_array[0],
	    text, param, reinterpret_cast<uptr>(stack), STACK_SIZE);

	t->state = thread::SLEEPING;
	sleeping_queue.insert_tail(t);

	*newthread = t;

	return cause::OK;
}

cause::stype thread_ctl::wakeup(thread* t)
{
	if (t->state == thread::READY)
		return cause::OK;

	sleeping_queue.remove(t);
	ready_queue.insert_tail(t);

	t->state = thread::READY;

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

void thread_ctl::sleep_running_thread()
{
	thread* prev_run = running_thread;

	processor& cpu = global_vars::gv.logical_cpu_obj_array[0];

	preempt_disable();

	if (prev_run->sleep_cancel_cmd == true) {
		prev_run->sleep_cancel_cmd = false;
		preempt_enable();
		return;
	}

	volatile thread::STATE* state = &prev_run->state;
	*state = thread::SLEEPING;
	sleeping_queue.insert_tail(prev_run);

	thread* next_run;
	for (;;) {
		next_run = ready_queue.remove_head();
		if (next_run)
			break;

		running_thread = 0;
		asm volatile ("sti;hlt":::"memory");

		if (*state == thread::READY) {
			return;
		} else {
			continue;
		}
	}

	running_thread = next_run;

	cpu.set_running_thread(next_run);
	switch_regset(next_run->ref_regset(), prev_run->ref_regset());

	preempt_enable();
}

void thread_ctl::ready_thread(thread* t)
{
	native::cli();

	if (t->state == thread::SLEEPING) {
		sleeping_queue.remove(t);
		ready_queue.insert_tail(t);
		t->state = thread::READY;
	} else {
		t->sleep_cancel_cmd = true;
	}

	native::sti();
}

void thread_ctl::switch_thread_after_intr(thread* t)
{
	if (running_thread)
		ready_queue.insert_tail(running_thread);

	ready_queue.remove(t);
	running_thread = t;

	processor& cpu = global_vars::gv.logical_cpu_obj_array[0];
	cpu.set_running_thread(t);
}

void thread_ctl::switch_messenger_thread_after_intr()
{
	ready_event_thread_in_intr();
	switch_thread_after_intr(event_thread);
}

void thread_ctl::ready_event_thread()
{
	thread* t = event_thread;

	native::cli();

	if (t->state == thread::SLEEPING) {
		sleeping_queue.remove(t);
		ready_queue.insert_tail(t);
		t->state = thread::READY;
	} else {
		t->sleep_cancel_cmd = true;
	}

	native::sti();
}

void thread_ctl::ready_event_thread_in_intr()
{
	thread* t = event_thread;

	if (t->state == thread::SLEEPING) {
		t->state = thread::READY;
		sleeping_queue.remove(t);

//		if (running_thread == 0) {
//			running_thread = t;
//			global_vars::gv.logical_cpu_obj_array[0]
//			    .set_running_thread(t);
//		} else {
			ready_queue.insert_tail(t);
//		}
	} else {
		t->sleep_cancel_cmd = true;
	}
}

