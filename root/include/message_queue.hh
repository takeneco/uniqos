/// @file   message_queue.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MESSAGE_QUEUE_HH_
#define INCLUDE_MESSAGE_QUEUE_HH_

#include <message.hh>


class message_queue
{
	bochain<message_item, &message_item::chain_hook> message_chain;

public:
	void push(message_item* e) { message_chain.insert_tail(e); }
	message_item* pop() { return message_chain.remove_head(); }

	bool probe() {
		return message_chain.head() != 0;
	}
};


#endif  // include guard

