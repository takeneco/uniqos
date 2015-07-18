/// @file  core/thread_sched.hh

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

#ifndef CORE_THREAD_SCHED_HH_
#define CORE_THREAD_SCHED_HH_

#include <core/thread.hh>


class cpu_node;
class mempool;

class thread_sched
{
public:
	thread_sched(cpu_node* _owner_cpu);

	void init();

	cause::pair<thread*> start();
	cause::t attach_boot_thread(thread* t);

	void attach(thread* t);
	void detach(thread* t);

	thread* sleep_current_thread_np();
	void ready(thread* t);
	void ready_np(thread* t);

	thread* get_running_thread() { return running_thread; }
	void set_running_thread(thread* t);

	thread* switch_next_thread();

	thread* exit_thread(thread* t);

void dump();
private:
	void _ready(thread* t);

private:
	cpu_node* const owner_cpu;

	thread* running_thread;

	spin_rwlock thread_state_lock;

	typedef fchain<thread, &thread::thread_sched_chainnode> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;
};


#endif  // include guard

