/// @file   event_queue.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_EVENT_QUEUE_HH_
#define INCLUDE_EVENT_QUEUE_HH_

#include "event.hh"


class event_queue
{
	dechain<event_item, &event_item::chain_hook> event_chain;
public:
	void push(event_item* e) { event_chain.insert_tail(e); }
	event_item* pop() { return event_chain.remove_head(); }
};


#endif  // include guard
