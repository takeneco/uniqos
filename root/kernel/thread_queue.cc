// @file   thread_queue.cc
// @brief  thread_queue class implements.

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <thread_queue.hh>

#include <global_vars.hh>
#include <mempool.hh>
#include <new_ops.hh>
#include <cpu_node.hh>

#include <log.hh>


namespace {

const uptr STACK_SIZE = 0x1000;

}


thread_queue::thread_queue(cpu_node* _owner) :
	owner_cpu(_owner),
	thread_mempool(0),
	stack_mempool(0)
{
}

cause::type thread_queue::init()
{
	cause::type r = mempool_acquire_shared(sizeof (thread),
	                                       &thread_mempool);
	if (is_fail(r))
		return r;

	r = mempool_acquire_shared(STACK_SIZE, &stack_mempool);
	if (is_fail(r))
		return r;

	thread* current = new (thread_mempool->alloc()) thread(get_cpu_node(),
	    0, 0, 0, 0);
	running_thread = current;

	return cause::OK;
}

cause::type thread_queue::create_thread(
    uptr text, uptr param, thread** newthread)
{
	void* stack = stack_mempool->alloc();
	if (!stack)
		return cause::NOMEM;

	void* p = thread_mempool->alloc();
	if (!p) {
		stack_mempool->dealloc(stack);
		return cause::NOMEM;
	}

	thread* t = new (p) thread(get_cpu_node(),
	    text, param, reinterpret_cast<uptr>(stack), STACK_SIZE);

	t->state = thread::SLEEPING;

	thread_state_lock.wlock();

	sleeping_queue.insert_tail(t);

	thread_state_lock.un_wlock();

	*newthread = t;

	return cause::OK;
}

extern "C" void switch_regset(arch::regset* r1, arch::regset* r2);

/// @brief  呼び出し元スレッドは READY のままでスレッドを切り替える。
/// @retval true スレッド切り替えの後、実行順が戻ってきた。
/// @retval false 切り替えるスレッドがなかった。
/// @note preempt_enable / intr_disable の状態で呼び出す必要がある。
bool thread_queue::force_switch_thread()
{
	thread* prev_thr = running_thread;
	thread* next_thr;

	{
		spin_wlock_section swl_sec(thread_state_lock, true);

		next_thr = ready_queue.remove_head();
		if (!next_thr)
			return false;

		ready_queue.insert_tail(running_thread);

		running_thread = next_thr;
		owner_cpu->set_running_thread(next_thr);
	}

	switch_regset(next_thr->ref_regset(), prev_thr->ref_regset());

	return true;
}

/// @brief  running_thread を SLEEPING にする。
void thread_queue::sleep()
{
	thread* prev_run = running_thread;
	thread* next_run;

	arch::intr_disable();

	{
		spin_wlock_section _tsl_sec(thread_state_lock, true);

		{
			spin_lock_section _asl_sec(prev_run->anti_sleep_lock,
			                           true);

			if (prev_run->anti_sleep == true) {
				prev_run->anti_sleep = false;
				arch::intr_enable();
				return;
			}

			prev_run->state = thread::SLEEPING;
			sleeping_queue.insert_tail(prev_run);
		}

		next_run = ready_queue.remove_head();
		// message_thread が常に READY なので、
		// next_run は 0 にならない。

		running_thread = next_run;
		owner_cpu->set_running_thread(next_run);
	}

	switch_regset(next_run->ref_regset(), prev_run->ref_regset());

	arch::intr_enable();
}

void thread_queue::ready(thread* t)
{
	preempt_disable_section _pds;

	_ready(t);
}

void thread_queue::ready_np(thread* t)
{
	_ready(t);
}

void thread_queue::switch_thread_after_intr(thread* t)
{
	spin_wlock_section _tsl_sec(thread_state_lock, true);

	ready_queue.insert_tail(running_thread);

	ready_queue.remove(t);
	running_thread = t;

	owner_cpu->set_running_thread(t);
}

void thread_queue::_ready(thread* t)
{
	spin_wlock_section _tsl_sec(thread_state_lock, true);
	spin_lock_section _sl_sec(t->anti_sleep_lock, true);

	if (t->state == thread::SLEEPING) {
		sleeping_queue.remove(t);
		ready_queue.insert_tail(t);
		t->state = thread::READY;
	} else {
		t->anti_sleep = true;
	}
}

