/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_variables.hh"


void drive()
{
	event_queue* queue = global_variable::gv.events;
	for (;;) {
		for (;;) {
			event_item* event = queue->pop();
			if (!event)
				break;

			event->handler(event->param);
		}

		arch::halt();
	}
}

