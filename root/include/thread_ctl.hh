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
	cause::stype wakeup(thread* t);

private:
	thread* active_thread;

	typedef bibochain<thread, &thread::chain_node> thread_chain;
	thread_chain running_queue;
	thread_chain sleeping_queue;

	mempool* thread_mempool;
	mempool* stack_mempool;
};


#endif  // include guard
