/// @file  cpu.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_CPU_HH_
#define INCLUDE_CPU_HH_

#include <basic.hh>
#include <cpu_ctl.hh>
#include <event_queue.hh>
#include <thread_ctl.hh>


/// Architecture independent part of processor control.
class processor : public arch::cpu_ctl
{
	DISALLOW_COPY_AND_ASSIGN(processor);

public:
	processor() {}

	cause::stype init();
	bool run_all_intr_event();

	void preempt_disable();
	void preempt_enable();

	void post_intr_event(event_item* ev);
	void post_soft_event(event_item* ev);

	void sleep_current_thread() { thrdctl.sleep_running_thread(); }

	thread_ctl& get_thread_ctl() { return thrdctl; }
	event_queue& get_soft_evq() { return soft_evq; }

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
};

processor* get_current_cpu();

void preempt_enable();
void preempt_disable();


#endif  // include guards
