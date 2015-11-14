/// @file   timer_liner_q.hh
/// @brief  タイマメッセージ用の線形キュー
//
// (C) 2013 KATO Takeshi
//

#ifndef INCLUDE_TIMER_LINER_Q_HH_
#define INCLUDE_TIMER_LINER_Q_HH_

#include <core/timer_ctl.hh>


class timer_liner_store : public timer_store
{
public:
	static cause::type setup();

	timer_liner_store();

	bool on_timer_store_Set(timer_message* new_msg);

	cause::pair<tick_time> on_timer_store_NextClock();

	cause::type on_timer_store_Post(tick_time clock);

private:
	typedef chain<timer_message, &timer_message::timer_store_chain_node>
	    message_chain;

	message_chain msg_chain;
};


#endif  // include guard

