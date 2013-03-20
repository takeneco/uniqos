/// @file   timer_liner_q.hh
/// @brief  タイマメッセージ用の線形キュー
//
// (C) 2013 KATO Takeshi
//

#ifndef INCLUDE_TIMER_LINER_Q_HH_
#define INCLUDE_TIMER_LINER_Q_HH_

#include <timer_ctl.hh>


class timer_liner_queue : public timer_queue
{
public:
	static cause::type setup();

	timer_liner_queue();

	bool on_timer_queue_Set(timer_message* new_msg);

	cause::pair<tick_time> on_timer_queue_NextClock();

	cause::type on_timer_queue_Post(tick_time clock);

private:
	typedef bibochain<timer_message, &timer_message::chain_hook>
	    message_chain;

	message_chain msg_chain;
};


#endif  // include guard

