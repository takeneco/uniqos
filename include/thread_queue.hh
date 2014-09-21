/// @file  thread_sched.hh
//
// (C) 2012-2014 KATO Takeshi
//

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

private:
	void _ready(thread* t);

private:
	cpu_node* const owner_cpu;

	thread* running_thread;

	spin_rwlock thread_state_lock;

	typedef bibochain<thread, &thread::thread_sched_chainnode> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;
};


#endif  // include guard

