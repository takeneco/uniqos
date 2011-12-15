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

	event_queue* evctl = global_vars::gv.event_ctl_obj;
	for (;;) {
		for (;;) {
			event_item* event = evctl->pop();
			if (!event)
				break;

			event->handler(event->param);
		}

		arch::halt();

		//log_target& klog = log();
		//serial_dump(((log_file&)klog).get_file());
	}
}

void post_event(event_item* event)
{
	event_queue* evctl = global_vars::gv.event_ctl_obj;

	evctl->push(event);
}

