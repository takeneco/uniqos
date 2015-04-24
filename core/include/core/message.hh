/// @file   message.hh
/// @brief  kernel internal message.
//
// (C) 2011-2015 KATO Takeshi
//

#ifndef CORE_MESSAGE_HH_
#define CORE_MESSAGE_HH_

#include <core/chain.hh>


class message
{
public:
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

	forward_chain_node<message> msgq_chain_node;
	handler_type handler;
};

template<class T>
class message_with : public message
{
public:
	T data;
};


#endif  // include guard

