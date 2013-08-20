/// @file   message_queue.hh
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef INCLUDE_MESSAGE_QUEUE_HH_
#define INCLUDE_MESSAGE_QUEUE_HH_

#include <message.hh>


class message_queue
{
	bochain<message, &message::chain_hook> msg_chain;

public:
	message_queue();
	~message_queue();

	void push(message* e) { msg_chain.push_back(e); }
	message* pop() { return msg_chain.pop_front(); }

	bool probe() {
		return msg_chain.head() != 0;
	}

	bool deliv_np();
	bool deliv_all_np();
};


#endif  // include guard

