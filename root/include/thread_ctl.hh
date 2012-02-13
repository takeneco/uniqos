/// @file  thread_ctl.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_CTL_HH_
#define INCLUDE_THREAD_CTL_HH_

#include <regset.hh>
#include <thread.hh>


class mempool;

class thread_ctl
{
public:
	thread_ctl();

	cause::stype init();

	cause::stype create_thread(uptr text, uptr param, thread** newthread);

	typedef void (*entry_point)(void* context);
	cause::stype create_thread(
	    entry_point func, void* context, thread** newthread) {
		return create_thread(reinterpret_cast<uptr>(func),
		                     reinterpret_cast<uptr>(context),
		                     newthread);
	}

	cause::stype wakeup(thread* t);
	///
	void manual_switch();
	///
	void sleep_running_thread();
	void ready_thread(thread* t);

	thread* get_running_thread() { return running_thread; }

	void set_event_thread(thread* t) { event_thread = t; }
	void ready_event_thread();
	void ready_event_thread_in_intr();

private:
	thread* running_thread;
	thread* event_thread;

	typedef bibochain<thread, &thread::chain_node> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;

	mempool* thread_mempool;
	mempool* stack_mempool;
};


#endif  // include guard
