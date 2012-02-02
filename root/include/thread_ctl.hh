/// @file  thread_ctl.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_CTL_HH_
#define INCLUDE_THREAD_CTL_HH_

#include <chain.hh>
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

private:
	thread* running_thread;

	typedef bibochain<thread, &thread::chain_node> thread_chain;
	thread_chain ready_queue;
	thread_chain sleeping_queue;

	mempool* thread_mempool;
	mempool* stack_mempool;
};


#endif  // include guard
