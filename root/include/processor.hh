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
#include <event_queue.hh>
#include <thread_ctl.hh>


class page_pool;

/// Architecture independent part of processor control.
class cpu_node : public arch::cpu_ctl
{
	DISALLOW_COPY_AND_ASSIGN(cpu_node);

public:
	cpu_node() {}

	cause::type set_page_pool_cnt(int cnt) {
		page_pool_cnt = cnt;
		return cause::OK;
	}
	cause::type set_page_pool(int pri, page_pool* pp);
	cause::stype init();
	bool run_all_intr_event();

	void preempt_disable();
	void preempt_enable();

	void post_intr_event(event_item* ev);
	void post_soft_event(event_item* ev);

	void sleep_current_thread() { thrdctl.sleep_running_thread(); }

	thread_ctl& get_thread_ctl() { return thrdctl; }
	event_queue& get_soft_evq() { return soft_evq; }

	cause::type page_alloc(arch::page::TYPE page_type, uptr* padr);
	cause::type page_dealloc(arch::page::TYPE page_type, uptr padr);

private:
	bool probe_intr_event();
	event_item* get_next_intr_event();

private:
	u8 preempt_disable_nests;

	thread_ctl thrdctl;

	/// 外部割込みによって発生したイベントを溜める。
	/// intr_evq を操作するときは CPU が割り込み禁止状態になっていなければ
	/// ならない。
	event_queue intr_evq;

	event_queue soft_evq;

	int        page_pool_cnt;
	page_pool* page_pools[CONFIG_MAX_CPUS];
};

int get_cpu_node_count();
cpu_node* get_cpu_node();
cpu_node* get_cpu_node(int cpuid);
cpu_node* get_current_cpu();

void preempt_enable();
void preempt_disable();


#endif  // include guards
