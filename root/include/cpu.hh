/// @file  cpu.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_CPU_HH_
#define INCLUDE_CPU_HH_

#include <basic.hh>
#include <event_queue.hh>
#include <thread_ctl.hh>


class basic_cpu
{
	DISALLOW_COPY_AND_ASSIGN(basic_cpu);

public:
	basic_cpu() {}

	cause::stype init();
	void run_intr_event();

	void post_intr_event(event_item* ev);

	thread_ctl& get_thread_ctl() { return thrdctl; }

private:
	event_item* get_next_intr_event();

private:
	thread_ctl thrdctl;

	/// 外部割込みによって発生したイベントを溜める。
	/// intr_evq を操作するときは CPU が割り込み禁止状態になっていなければ
	/// ならない。
	event_queue intr_evq;

	event_queue soft_evq;
};


#endif  // include guards
