/// @file  thread.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_THREAD_HH_
#define INCLUDE_THREAD_HH_

#include <basic.hh>
#include <chain.hh>
#include <regset.hh>


class processor;

class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_ctl;

public:
	thread(processor* _owner,
	    uptr text, uptr param, uptr stack, uptr stack_size);

	void ready();

	arch::regset* ref_regset() { return &rs; }

	bichain_node<thread>& chain_node() { return _chain_node; }

private:
	bichain_node<thread> _chain_node;
	processor* owner;

	arch::regset rs;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	bool sleep_cancel_cmd;
};


#endif  // include guard
