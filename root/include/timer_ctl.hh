/// @file  timer_ctl.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_TIMER_CTL_HH_
#define INCLUDE_TIMER_CTL_HH_

#include <basic.hh>
#include <config.h>
#include <message.hh>


class timer_message : public message
{
	typedef chain_node<timer_message> chain_node_type;

	chain_node_type _chain_node;

public:
	chain_node_type& chain_hook() { return _chain_node; }

	u64 expires_ns;
};


template <class T>
class timer_message_with : public timer_message
{
public:
	T data;
};


class timer_ctl
{
	DISALLOW_COPY_AND_ASSIGN(timer_ctl);

public:
	timer_ctl();

	cause::type setup_cpu();

private:
	class timer_queue;
	timer_queue* queue[CONFIG_MAX_CPUS];
};


#endif  // include guard

