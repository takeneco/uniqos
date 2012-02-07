/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include <cpu_ctl.hh>
#include "event_queue.hh"
#include "global_vars.hh"
#include "native_ops.hh"

#include "log.hh"


void test();

void event_drive()
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	for (;;) {
		event_item* event = evctl->pop();
		if (!event)
			break;

		event->handler(event->param);
	}
}

void drive()
{
	log()("(C) KATO Takeshi")();

	basic_cpu* cpu = arch::get_current_cpu();

	for (;;) {
		cpu->run_intr_event();

		//test();

		native::cli();
		event_drive();
		asm volatile ("sti;hlt");
	}
}

void post_event(event_item* event)
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	evctl->push(event);
}

