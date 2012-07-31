/// @file   message.hh
/// @brief  kernel internal message.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MESSAGE_HH_
#define INCLUDE_MESSAGE_HH_

#include <chain.hh>


class message
{
	chain_node<message> chain_node_;
public:
	chain_node<message>& chain_hook() { return chain_node_; }

	typedef void (*handler_type)(message* msg);
	handler_type handler;
};

template<class T>
class message_with : public message
{
public:
	T data;
};


void post_message(message* event);


#endif  // include guard

