/// @file   event.hh
/// @brief  kernel internal event.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_EVENT_HH_
#define INCLUDE_EVENT_HH_

#include "chain.hh"


class event_item
{
	chain_node<event_item> chain_node_;
public:
	chain_node<event_item>& chain_hook() { return chain_node_; }

	typedef void (*event_handler)(void* param);
	event_handler handler;
	void* param;
};

void post_event(event_item* event);


#endif  // include guard
