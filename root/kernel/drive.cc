/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_vars.hh"
#include "native_ops.hh"
#include <processor.hh>

#include "log.hh"


void test();

bool event_drive()
{
	event_queue* evctl = &global_vars::gv.logical_cpu_obj_array[0].
	    get_soft_evq();

	if (!evctl->probe())
		return false;

	preempt_disable();

	event_item* event = evctl->pop();

	preempt_enable();

	//// no effect
	//if (!event)
	//	break;

	event->handler(event->param);

	return true;
}

void drive()
{
	processor* cpu = arch::get_current_cpu();

	for (;;) {
		bool soft_ev = event_drive();

		arch::intr_disable();

		bool intr_ev = cpu->run_all_intr_event();

		if (soft_ev || intr_ev)
			arch::intr_enable();
		else
			// 割り込み禁止の状態で呼び出す。
			// x86依存
			cpu->sleep_current_thread();
	}
}

void post_event(event_item* event)
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	evctl->push(event);
}

