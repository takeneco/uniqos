/// @file   message_queue.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MESSAGE_QUEUE_HH_
#define INCLUDE_MESSAGE_QUEUE_HH_

#include <message.hh>


class message_queue
{
	bochain<message, &message::chain_hook> msg_chain;

public:
	void push(message* e) { msg_chain.insert_tail(e); }
	message* pop() { return msg_chain.remove_head(); }

	bool probe() {
		return msg_chain.head() != 0;
	}
};


#endif  // include guard

