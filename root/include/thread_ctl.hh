/// @file  thread_queue.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_QUEUE_HH_
#define INCLUDE_THREAD_QUEUE_HH_

#include <regset.hh>
#include <thread.hh>


class cpu_node;
class mempool;

class thread_queue
{
public:
	thread_queue(cpu_node* _owner);

	cause::type init();

	cause::type create_thread(uptr text, uptr param, thread** newthread);

	typedef void (*entry_point)(void* context);
	cause::type create_thread(entry_point func,
	                          void* context,
	                          thread** newthread) {
		return create_thread(reinterpret_cast<uptr>(func),
		                     reinterpret_cast<uptr>(context),
		                     newthread);
	}

	cause::type wakeup(thread* t);

	bool force_switch_thread();

	void sleep();
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

	typedef bibochain<thread, &thread::chain_node> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;

	mempool* thread_mempool;
	mempool* stack_mempool;
};


#endif  // include guard

