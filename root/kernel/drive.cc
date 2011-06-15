/// @file   drive.cc
/// @brief  kernel process.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "event_queue.hh"
#include "global_variables.hh"


////
#include "log.hh"
kern_output* kern_get_out();
void serial_dump(void*);
////

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

		//kern_output* kout = kern_get_out();
		//kout->put_c('.');
		kernel_log& klog = log();
		serial_dump(((log_file&)klog).get_file());
	}
}

void post_event(event_item* event)
{
	event_queue* queue = global_variable::gv.events;

	queue->push(event);
}

