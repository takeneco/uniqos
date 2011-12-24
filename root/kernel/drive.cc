/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_vars.hh"
#include "native_ops.hh"

#include "log.hh"


void test();

void event_drive()
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	for (;;) {
		native::cli();
		event_item* event = evctl->pop();
		native::sti();
		if (!event)
			break;

		event->handler(event->param);
	}
}

void drive()
{
	log()("(C) KATO Takeshi")();

	for (;;) {
		event_drive();

		//test();
		arch::halt();
	}
}

void post_event(event_item* event)
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	evctl->push(event);
}

