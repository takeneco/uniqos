/// @file  thread.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_HH_
#define INCLUDE_THREAD_HH_

#include <basic.hh>
#include <chain.hh>
#include <regset.hh>
#include <spinlock.hh>


class cpu_node;

class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_queue;

public:
	thread(cpu_node* _owner,
	    uptr text, uptr param, uptr stack, uptr stack_size);

	void ready();

	arch::regset* ref_regset() { return &rs; }

	bichain_node<thread>& chain_node() { return _chain_node; }

private:
	bichain_node<thread> _chain_node;
	cpu_node* owner;

	arch::regset rs;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	bool      anti_sleep;
	spin_lock anti_sleep_lock;
};


#endif  // include guard

