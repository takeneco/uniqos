/// @file  timer_ctl.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_TIMER_CTL_HH_
#define INCLUDE_TIMER_CTL_HH_

#include <basic.hh>
#include <clock_src.hh>
#include <config.h>
#include <message.hh>
#include <spinlock.hh>


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

	void set_clock_source(clock_source* cs);
	cause::type get_jiffy_tick(tick_time* tick);

private:
	clock_source* clk_src;

// timer_message database
public:

private:
	spin_lock lock;
	tick_time devtick_jiffy;

	enum {
		VEC_SIZE = 256,
		STEP_SIZE = 5,
	};
	typedef chain<timer_message, &timer_message::chain_hook>
	    message_chain;
	struct message_chain_vec {
		message_chain v[VEC_SIZE];
	};
	message_chain_vec step[STEP_SIZE];
};

cause::type get_jiffy_tick(tick_time* tick);


#endif  // include guard

