/// @file  cpu_node.hh
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef CORE_CPU_NODE_HH_
#define CORE_CPU_NODE_HH_

#include <arch.hh>
#include <core/basic.hh>
#include <config.h>
#include <message_queue.hh>
#include <thread_queue.hh>


class page_pool;

/// Architecture independent part of cpu control.
class cpu_node
{
	DISALLOW_COPY_AND_ASSIGN(cpu_node);

public:
	cpu_node();

	cause::t set_page_pool_cnt(int cnt);
	cause::t set_page_pool(int pri, page_pool* pp);

	cause::t setup();

	void     attach_thread(thread* t);
	cause::t detach_thread(thread* t);
	void     ready_thread(thread* t);
	void     ready_thread_np(thread* t);

	void force_set_running_thread(thread* t);

	thread_sched& get_thread_ctl() { return threads; }

	cause::t page_alloc(arch::page::TYPE page_type, uptr* padr);
	cause::t page_dealloc(arch::page::TYPE page_type, uptr padr);

private:
	static void preempt_wait();

protected:
	thread_sched threads;
	cpu_id_t     page_pool_cnt;
	page_pool* page_pools[CONFIG_MAX_CPUS];
};

cpu_id_t get_cpu_node_count();
cpu_node* get_cpu_node();
cpu_node* get_cpu_node(cpu_id_t cpuid);

namespace arch {
void post_intr_message(message* msg);
void post_cpu_message(message* msg);
void post_cpu_message(message* msg, cpu_node* cpu);
}  // namespace arch

void post_message(message* msg); //TODO:OBSOLETED

void preempt_disable();
void preempt_enable();

class preempt_disable_section
{
	cpu_node* cn;
public:
	preempt_disable_section();
	~preempt_disable_section();
};

class preempt_enable_section
{
	cpu_node* cn;
public:
	preempt_enable_section();
	~preempt_enable_section();
};


#endif  // include guard

