// @file   thread_queue.cc
// @brief  thread_queue class implements.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
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

// TODO: include from arch
#include <native_cpu_node.hh>
#include <native_thread.hh>

namespace {

const uptr STACK_SIZE = 0x1000;

}


thread_queue::thread_queue(cpu_node* _owner_cpu) :
	owner_cpu(_owner_cpu),
	running_thread(0)
{
}

cause::t thread_queue::init()
{
	return cause::OK;
}

/// @brief カーネルの初期化処理用スレッドを関連付ける。
//
/// 何もスレッドが無い状態で呼び出さなければならない。
cause::t thread_queue::attach_boot_thread(thread* t)
{
	running_thread = t;

	return cause::OK;
}

void thread_queue::attach(thread* t)
{
	t->owner_cpu = owner_cpu;

	thread_state_lock.wlock();

	if (t->state == thread::SLEEPING)
		sleeping_queue.insert_tail(t);
	else
		ready_queue.insert_tail(t);

	thread_state_lock.un_wlock();
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
		spin_wlock_section_np swl_sec(thread_state_lock);

		next_thr = ready_queue.remove_head();
		if (!next_thr)
			return false;

		ready_queue.insert_tail(running_thread);

		running_thread = next_thr;
		static_cast<x86::native_cpu_node*>(owner_cpu)->set_running_thread(next_thr);
	}

	// TODO: refs arch
	switch_regset(
	    static_cast<x86::native_thread*>(next_thr)->ref_regset(),
	    static_cast<x86::native_thread*>(prev_thr)->ref_regset());

	return true;
}

/// @brief  running_thread を SLEEPING にする。
void thread_queue::sleep()
{
	thread* prev_run = running_thread;
	thread* next_run;

	arch::intr_disable();

	{
		spin_wlock_section_np _tsl_sec(thread_state_lock);

		{
			spin_lock_section_np _asl_sec(
			    prev_run->anti_sleep_lock);

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
		static_cast<x86::native_cpu_node*>(owner_cpu)->set_running_thread(next_run);
	}

	// TODO: refs arch
	switch_regset(
	    static_cast<x86::native_thread*>(next_run)->ref_regset(),
	    static_cast<x86::native_thread*>(prev_run)->ref_regset());

	arch::intr_enable();
}

/// @brief Make current running thread sleeping.
/// @return Returns next running thread.
///         Returns 0 if sleep was canceled.
thread* thread_queue::sleep_current_thread()
{
	thread* prev_run = running_thread;

	spin_wlock_section_np _tsl_sec(thread_state_lock);
	{
		spin_lock_section_np _asl_sec(
		    prev_run->anti_sleep_lock);

		if (prev_run->anti_sleep == true) {
			prev_run->anti_sleep = false;
			return 0;
		}

		prev_run->state = thread::SLEEPING;
		sleeping_queue.insert_tail(prev_run);
	}

	running_thread = ready_queue.remove_head();
	// message_thread が常に READY なので 0 にならない。

	return running_thread;
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
	spin_wlock_section_np _tsl_sec(thread_state_lock);

	ready_queue.insert_tail(running_thread);

	ready_queue.remove(t);
	running_thread = t;

	static_cast<x86::native_cpu_node*>(owner_cpu)->set_running_thread(t);
}

void thread_queue::_ready(thread* t)
{
	spin_wlock_section_np _tsl_sec(thread_state_lock);
	spin_lock_section_np _sl_sec(t->anti_sleep_lock);

	if (t->state == thread::SLEEPING) {
		sleeping_queue.remove(t);
		ready_queue.insert_tail(t);
		t->state = thread::READY;
	} else {
		t->anti_sleep = true;
	}
}

