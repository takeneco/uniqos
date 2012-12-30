/// @file  cpu_node.hh
//
// (C) 2012 KATO Takeshi
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

	cause::type start_message_loop();
	void ready_messenger();
	void ready_messenger_np();
	void switch_messenger_after_intr();

	bool run_all_intr_message();

	u8   inc_preempt_disable() { return ++preempt_disable_cnt; }
	u8   dec_preempt_disable() { return --preempt_disable_cnt; }

	void post_intr_message(message* ev);
	void post_soft_message(message* ev);

	thread_queue& get_thread_ctl() { return thread_q; }
	message_queue& get_soft_msgq() { return soft_msgq; }

	cause::type page_alloc(arch::page::TYPE page_type, uptr* padr);
	cause::type page_dealloc(arch::page::TYPE page_type, uptr padr);

private:
	static void preempt_wait();

	bool probe_intr_message();
	message* get_next_intr_message();

	bool run_message();
	void message_loop();
	static void message_loop_entry(void* _cpu_node);

private:

#if CONFIG_PREEMPT
	s8 preempt_disable_cnt;
#endif  // CONFIG_PREEMPT

	thread_queue thread_q;

	thread* message_thread;

	/// 外部割込みによって発生したイベントを溜める。
	/// intr_evq を操作するときは CPU が割り込み禁止状態になっていなければ
	/// ならない。
	message_queue intr_msgq;

	message_queue soft_msgq;

	cpu_id     page_pool_cnt;
	page_pool* page_pools[CONFIG_MAX_CPUS];

public:
	s8 _ref_preempt_disable_cnt() const { return preempt_disable_cnt; }
};

cpu_id get_cpu_node_count();
cpu_node* get_cpu_node();
cpu_node* get_cpu_node(cpu_id cpuid);

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

