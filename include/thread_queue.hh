/// @file  thread_sched.hh
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef INCLUDE_THREAD_SCHED_HH_
#define INCLUDE_THREAD_SCHED_HH_

#include <thread.hh>


class cpu_node;
class mempool;

class thread_sched
{
public:
	thread_sched(cpu_node* _owner_cpu);

	cause::t init();
	cause::t attach_boot_thread(thread* t);

	void attach(thread* t);

	thread* sleep_current_thread();
	void ready(thread* t);
	void ready_np(thread* t);
	void ready_thread(thread* t);

	thread* get_running_thread() { return running_thread; }
	void set_running_thread(thread* t);

	thread* switch_next_thread();

private:
	void _ready(thread* t);

private:
	cpu_node* const owner_cpu;

	thread* running_thread;

	spin_rwlock thread_state_lock;

	typedef bibochain<thread, &thread::chain_node> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;
};

//TODO
typedef thread_sched thread_queue;


#endif  // include guard

