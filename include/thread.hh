/// @file  thread.hh
/// @brief thread class declaration.
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef CORE_THREAD_HH_
#define CORE_THREAD_HH_

#include <core/basic.hh>
#include <core/spinlock.hh>


class cpu_node;
class process;

typedef u32 thread_id;

class thread
{
	DISALLOW_COPY_AND_ASSIGN(thread);

	friend class thread_sched;

public:
	thread(thread_id tid);

	cpu_node* get_owner_cpu() { return owner_cpu; }
	process* get_owner_process() { return owner_process; }
	void set_owner_process(process* p) { owner_process = p; }
	thread_id get_thread_id() const { return id; }

	void ready();

	bichain_node<thread>& thread_sched_chainnode() {
		return _thread_sched_chainnode;
	}
	bichain_node<thread>& process_chainnode() {
		return _process_chainnode;
	}

private:
	cpu_node* owner_cpu;
	process* owner_process;

	thread_id id;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	/// locked by thread_queue::thread_state_lock
	bool      anti_sleep;

	bichain_node<thread> _thread_sched_chainnode;
	bichain_node<thread> _process_chainnode;
};

thread* get_current_thread();
void sleep_current_thread();


#endif  // include guard

