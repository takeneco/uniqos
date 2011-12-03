/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_vars.hh"


////
#include "log.hh"
void serial_dump(void*);
////

void drive()
{
	log()("(C) KATO Takeshi")();

	event_queue* queue = global_vars::gv.events;
	for (;;) {
		for (;;) {
			event_item* event = queue->pop();
			if (!event)
				break;

			event->handler(event->param);
		}

		arch::halt();

		log_target& klog = log();
		serial_dump(((log_file&)klog).get_file());
	}
}

void post_event(event_item* event)
{
	event_queue* queue = global_vars::gv.events;

	queue->push(event);
}

