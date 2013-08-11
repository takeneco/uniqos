/// @file  thread_queue.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_THREAD_QUEUE_HH_
#define INCLUDE_THREAD_QUEUE_HH_

#include <thread.hh>


class cpu_node;
class mempool;

class thread_queue
{
public:
	thread_queue(cpu_node* _owner_cpu);

	cause::type init();
	cause::t attach_boot_thread(thread* t);

	void attach(thread* t);

	bool force_switch_thread();

	thread* sleep_current_thread();
	void ready(thread* t);
	void ready_np(thread* t);
	void ready_thread(thread* t);
	void switch_thread_after_intr(thread* t);

	thread* get_running_thread() { return running_thread; }

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


#endif  // include guard

