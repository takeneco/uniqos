/// @file   core/include/core/timer.hh
/// @brief  Timer interfaces.
//
// (C) 2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_TIMER_HH_
#define CORE_INCLUDE_CORE_TIMER_HH_

#include <basic.hh>
#include <message.hh>


enum { TICK_HZ = 1000000000, };

class timer_message : public message
{
	friend class timer_ctl;

	typedef bichain_node<timer_message> chain_node_type;

	chain_node_type _chain_node;

public:
	chain_node_type& chain_hook() { return _chain_node; }

	tick_time nanosec_delay;

private:
public:
	tick_time expires_clock;
};


template <class T>
class timer_message_with : public timer_message
{
public:
	T data;
};


cause::t timer_set(timer_message* m);


#endif  // include guard
