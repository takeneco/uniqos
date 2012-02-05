/// @file  thread.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_HH_
#define INCLUDE_THREAD_HH_

#include <basic.hh>
#include <chain.hh>
#include <regset.hh>


class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_ctl;

public:
	thread(uptr text, uptr param, uptr stack, uptr stack_size);
	arch::regset* ref_regset() { return &rs; }

	bichain_node<thread>& chain_node() { return _chain_node; }

private:
	bichain_node<thread> _chain_node;

	arch::regset rs;

	enum {
		RUNNING,
		SLEEPING,
	} state;
};


#endif  // include guard
