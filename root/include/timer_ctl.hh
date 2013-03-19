/// @file  timer_ctl.hh
//
// (C) 2012-2013 KATO Takeshi
//

/// タイマデバイスが刻む時刻を clock と呼び、
/// カーネル内時刻を tick と呼ぶことにする。

#ifndef INCLUDE_TIMER_CTL_HH_
#define INCLUDE_TIMER_CTL_HH_

#include <basic.hh>
#include <clock_src.hh>
#include <config.h>
#include <message.hh>
#include <spinlock.hh>


class output_buffer;

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


class timer_queue
{
	DISALLOW_COPY_AND_ASSIGN(timer_queue);

public:
	timer_queue() {}

protected:
public: //TODO:operationsをdynamicに作ればpublicを外せる
	struct operations
	{
		void init();

		typedef bool (*SetOP)(
		    timer_queue* x, timer_message* msg);
		SetOP Set;

		typedef cause::pair<tick_time> (*NextClockOP)(
		    timer_queue* x);
		NextClockOP NextClock;

		typedef cause::type (*PostOP)(
		    timer_queue* x, tick_time clock);
		PostOP Post;
	};

	template<class X> static bool call_on_timer_queue_Set(
	    timer_queue* x, timer_message* msg) {
		return static_cast<X*>(x)->on_timer_queue_Set(msg);
	}

	template<class X> static cause::pair<tick_time>
	call_on_timer_queue_NextClock(timer_queue* x) {
		return static_cast<X*>(x)->on_timer_queue_NextClock();
	}

	template<class X> static cause::type call_on_timer_queue_Post(
	    timer_queue* x, tick_time clock) {
		return static_cast<X*>(x)->on_timer_queue_Post(clock);
	}

public:
	bool set(timer_message* msg) {
		return ops->Set(this, msg);
	}

	cause::pair<tick_time> next_clock() {
		return ops->NextClock(this);
	}

	cause::type post(tick_time clock) {
		return ops->Post(this, clock);
	}

	operations* ops;
};


class timer_ctl
{
	DISALLOW_COPY_AND_ASSIGN(timer_ctl);

public:
	timer_ctl();

	void set_clock_source(clock_source* cs);
	void set_queue(timer_queue* tq);
	cause::type get_jiffy_tick(tick_time* tick);

private:
	clock_source* clk_src;

// timer_message database
public:
	cause::type set_timer(timer_message* msg);

	void on_timer_message();

private:
	cause::type _set_timer(timer_message* msg, tick_time now_clock);

private:
	spin_lock lock;

	message_with<timer_ctl*> timer_msg;

	timer_queue* queue;

public:
	void dump(output_buffer& ob);
};

cause::type get_jiffy_tick(tick_time* tick);


#endif  // include guard

