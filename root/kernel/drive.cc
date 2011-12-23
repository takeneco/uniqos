/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_vars.hh"

#include "log.hh"

void drive()
{
	log()("(C) KATO Takeshi")();

	event_queue* evctl = global_vars::gv.event_ctl_obj;
	for (;;) {
		for (;;) {
			event_item* event = evctl->pop();
			if (!event)
				break;

			event->handler(event->param);
		}

		arch::halt();
	}
}

void post_event(event_item* event)
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	evctl->push(event);
}

