/// @file   message.hh
/// @brief  kernel internal message.
//
// (C) 2011-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_MESSAGE_HH_
#define CORE_INCLUDE_CORE_MESSAGE_HH_

#include <core/chain.hh>


class message
{
	chain_node<message> chain_node_;
public:
	chain_node<message>& chain_hook() { return chain_node_; }

	typedef void (*handler_type)(message* msg);

	message()
#if CONFIG_DEBUG_VALIDATE > 0
		: handler(0)
#endif  // CONFIG_DEBUG_VALIDATE > 0
	{
	}
	explicit message(handler_type ht)
		: handler(ht)
	{
	}

	handler_type handler;
};

template<class T>
class message_with : public message
{
public:
	T data;
};


#endif  // include guard

