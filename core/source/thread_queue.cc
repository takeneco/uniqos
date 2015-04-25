// @file   thread_sched.cc
// @brief  thread_sched class implements.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <core/thread_sched.hh>

#include <core/cpu_node.hh>
#include <core/new_ops.hh>

/** @class thread_sched
 *
 * thread_sched は実行する thread を選択するだけなので、thread を切り替える
 * 処理は別の場所で実装する必要がある。
 */

/** @brief thread_sched によるスケジューリングのはじめ方
 *
 * 2通りの方法がある。
 * (1) start()
 * (2) attach_boot_thread()
 */

thread_sched::thread_sched(cpu_node* _owner_cpu) :
	owner_cpu(_owner_cpu),
	running_thread(0)
{
}

void thread_sched::init()
{
}

cause::pair<thread*> thread_sched::start()
{
	running_thread = ready_queue.pop_front();

	if (running_thread)
		return make_pair(cause::OK, running_thread);
	else
		return null_pair(cause::FAIL);
}

/// @brief カーネルの初期化処理用スレッドを関連付ける。
//
/// 何もスレッドが無い状態で呼び出さなければならない。
cause::t thread_sched::attach_boot_thread(thread* t)
{
	t->owner_cpu_node_id = owner_cpu->get_cpu_node_id();
	t->owner_cpu = owner_cpu;
	t->state = thread::READY;

	running_thread = t;

	return cause::OK;
}

void thread_sched::attach(thread* t)
{
	t->owner_cpu_node_id = owner_cpu->get_cpu_node_id();
	t->owner_cpu = owner_cpu;

	thread_state_lock.wlock();

	if (t->state == thread::SLEEPING)
		sleeping_queue.push_back(t);
	else
		ready_queue.push_back(t);

	thread_state_lock.un_wlock();
}

void thread_sched::detach(thread* t)
{
	thread_state_lock.wlock();

	if (t->state == thread::SLEEPING)
		sleeping_queue.remove(t);
	else
		ready_queue.remove(t);

	thread_state_lock.un_wlock();
}

/// @brief Make current running thread sleeping.
/// @return Returns next running thread.
///         Returns 0 if sleep was canceled.
thread* thread_sched::sleep_current_thread_np()
{
	thread* prev_run = running_thread;

	spin_wlock_section_np _tsl_sec(thread_state_lock);
	{
		if (prev_run->anti_sleep == true) {
			prev_run->anti_sleep = false;
			return 0;
		}

		prev_run->state = thread::SLEEPING;
		sleeping_queue.push_back(prev_run);
	}

	running_thread = ready_queue.pop_front();
	// message_thread が常に READY なので 0 にならない。

	return running_thread;
}

void thread_sched::ready(thread* t)
{
	preempt_disable_section _pds;

	_ready(t);
}

void thread_sched::ready_np(thread* t)
{
	_ready(t);
}

/// @brief  Change running thread ptr.
//
/// この関数は running thread のポインタを更新するだけなので、
/// スレッドを切り替える実装は呼び出し元に書く必要がある。
void thread_sched::set_running_thread(thread* t)
{
	if (CONFIG_DEBUG_VALIDATE > 0) {
		if (!(t->state == thread::READY &&
		      t->owner_cpu == this->owner_cpu &&
		      t != running_thread))
		{
			/* TODO:スレッドが変わらないのにこの関数を呼び出すこと
			 * をよしとするか？
			log()(SRCPOS)(" Call fault:t=")(t)('@').
			    u(t->state == thread::READY).
			    u(t->owner_cpu == this->owner_cpu).
			    u(t != running_thread)();
			*/
		}
	}

	spin_wlock_section_np _tsl_sec(thread_state_lock);

	ready_queue.push_back(running_thread);

	ready_queue.remove(t);
	running_thread = t;
}

/// @brief  Change running thread ptr to next thread.
/// @return This function returns new running thread ptr.
//
/// この関数は running thread のポインタを更新するだけなので、
/// スレッドを切り替える実装は呼び出し元に書く必要がある。
thread* thread_sched::switch_next_thread()
{
	spin_wlock_section_np _swl_sec(thread_state_lock);

	thread* next_thr = ready_queue.pop_front();
	if (!next_thr)
		return 0;

	ready_queue.push_back(running_thread);

	running_thread = next_thr;

	return next_thr;
}

/// @brief Exit thread.
/// @return This function returns new running thread ptr.
thread* thread_sched::exit_thread(thread* t)
{
	spin_wlock_section_np _tsl_sec(thread_state_lock);

	if (t->state == thread::READY) {
		if (t == running_thread)
			running_thread = ready_queue.pop_front();
		else
			ready_queue.remove(t);
	} else {
		sleeping_queue.remove(t);
	}

	return running_thread;
}

/// @brief Make thread ready.
void thread_sched::_ready(thread* t)
{
	spin_wlock_section_np _tsl_sec(thread_state_lock);

	if (t->state == thread::SLEEPING) {
		sleeping_queue.remove(t);
		ready_queue.push_back(t);
		t->state = thread::READY;
	} else {
		t->anti_sleep = true;
	}
}

#include <core/log.hh>
void thread_sched::dump()
{
	log(1)("run=")(running_thread)();
	log(1)("sleep:");
	for(auto i = sleeping_queue.front();
            i;
	    i = sleeping_queue.next(i))
	{
		log(1)(" ")(i);
	}
	log(1)()("ready:");
	for(auto i = ready_queue.front();
            i;
	    i = ready_queue.next(i))
	{
		log(1)(" ")(i);
	}
	log(1)()("--- thread_sched dump end ---")();
}

