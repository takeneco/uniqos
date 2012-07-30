/// @file   message.hh
/// @brief  kernel internal message.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MESSAGE_HH_
#define INCLUDE_MESSAGE_HH_

#include <chain.hh>


class message_item
{
	chain_node<message_item> chain_node_;
public:
	chain_node<message_item>& chain_hook() { return chain_node_; }

	typedef void (*message_handler)(void* param);
	message_handler handler;
	void* param;
};

void post_message(message_item* event);


#endif  // include guard

