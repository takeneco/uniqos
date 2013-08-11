/// @file  thread.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_THREAD_HH_
#define INCLUDE_THREAD_HH_

#include <basic.hh>
#include <chain.hh>
#include <spinlock.hh>


class cpu_node;

class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_queue;

public:
	thread();

	void ready();

	bichain_node<thread>& chain_node() { return _chain_node; }

private:
	bichain_node<thread> _chain_node;
	cpu_node* owner_cpu;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	bool      anti_sleep;
	spin_lock anti_sleep_lock;
};

void sleep_current_thread();


#endif  // include guard

