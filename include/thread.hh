/// @file  thread.hh
/// @brief thread class declaration.
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_THREAD_HH_
#define CORE_INCLUDE_CORE_THREAD_HH_

#include <core/basic.hh>
#include <spinlock.hh>


class cpu_node;

class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_sched;

public:
	thread();

	cpu_node* get_owner_cpu() { return owner_cpu; }

	void ready();

	bichain_node<thread>& chain_node() { return _chain_node; }

private:
	bichain_node<thread> _chain_node;
	cpu_node* owner_cpu;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	/// locked by thread_queue::thread_state_lock
	bool      anti_sleep;
};

thread* get_current_thread();
void sleep_current_thread();


#endif  // include guard

