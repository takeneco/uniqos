/// @file   message_queue.hh
//
// (C) 2011-2015 KATO Takeshi
//

#ifndef CORE_MESSAGE_QUEUE_HH_
#define CORE_MESSAGE_QUEUE_HH_

#include <core/message.hh>


class message_queue
{
public:
	message_queue();
	~message_queue();

	void push(message* e) { msg_chain.push_back(e); }
	message* pop() { return msg_chain.pop_front(); }

	bool probe() {
		return msg_chain.front() != 0;
	}

	bool deliv_np();
	bool deliv_all_np();

private:
	forward_chain<message, &message::msgq_chain_node> msg_chain;
};


#endif  // include guard

