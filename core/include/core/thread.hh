/// @file  thread.hh
/// @brief thread class declaration.
//
// (C) 2012-2015 KATO Takeshi
//

#ifndef CORE_THREAD_HH_
#define CORE_THREAD_HH_

#include <arch.hh>
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

	cpu_id   get_owner_cpu_node_id() const { return owner_cpu_node_id; }
	cpu_node* get_owner_cpu() { return owner_cpu; }
	process* get_owner_process() { return owner_process; }
	void set_owner_process(process* p) { owner_process = p; }
	thread_id get_thread_id() const { return id; }

	void ready();

	chain_node<thread>& thread_sched_chainnode() {
		return _thread_sched_chainnode;
	}
	chain_node<thread>& process_chainnode() {
		return _process_chainnode;
	}

	void imitate_owner_cpu() { owner_cpu_node_id = 0; }

private:
	cpu_id owner_cpu_node_id;
	cpu_node* owner_cpu;
	process* owner_process;

	thread_id id;

	enum STATE {
		READY,
		SLEEPING,
	} state;

	/// locked by thread_queue::thread_state_lock
	bool      anti_sleep;

	chain_node<thread> _thread_sched_chainnode;
	chain_node<thread> _process_chainnode;
};

thread* get_current_thread();
void sleep_current_thread();


#endif  // CORE_THREAD_HH_

