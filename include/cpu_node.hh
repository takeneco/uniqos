/// @file  cpu_node.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_CPU_NODE_HH_
#define INCLUDE_CPU_NODE_HH_

#include <arch.hh>
#include <basic.hh>
#include <config.h>
#include <cpu_ctl.hh>
#include <message_queue.hh>
#include <thread_queue.hh>


class page_pool;

/// Architecture independent part of cpu control.
class cpu_node : public arch::cpu_ctl
{
	DISALLOW_COPY_AND_ASSIGN(cpu_node);

public:
	cpu_node();

	cause::type set_page_pool_cnt(int cnt);
	cause::type set_page_pool(int pri, page_pool* pp);

	cause::type setup();

	bool run_all_intr_message();

	thread_queue& get_thread_ctl() { return thread_q; }
	thread_queue& get_thread_queue() { return thread_q; }

	cause::type page_alloc(arch::page::TYPE page_type, uptr* padr);
	cause::type page_dealloc(arch::page::TYPE page_type, uptr padr);

private:
	static void preempt_wait();

protected:
	thread_queue thread_q;
private:
	cpu_id     page_pool_cnt;
	page_pool* page_pools[CONFIG_MAX_CPUS];
};

cpu_id get_cpu_node_count();
cpu_node* get_cpu_node();
cpu_node* get_cpu_node(cpu_id cpuid);

namespace arch {
void post_intr_message(message* msg);
void post_cpu_message(message* msg);
void post_cpu_message(message* msg, cpu_node* cpu);
}  // namespace arch
void post_message(message* msg);

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

