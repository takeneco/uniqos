/// @file  timer_ctl.hh
//
// (C) 2012-2013 KATO Takeshi
//

/// タイマデバイスが刻む時刻を clock と呼び、
/// カーネル内時刻を tick と呼ぶことにする。

#ifndef INCLUDE_TIMER_CTL_HH_
#define INCLUDE_TIMER_CTL_HH_

#include <clock_src.hh>
#include <config.h>
#include <core/timer.hh>
#include <spinlock.hh>


class output_buffer;

class timer_store
{
	DISALLOW_COPY_AND_ASSIGN(timer_store);

public:
	timer_store() {}

protected:
public: //TODO:operationsをdynamicに作ればpublicを外せる
	struct operations
	{
		void init();

		typedef bool (*SetOP)(
		    timer_store* x, timer_message* msg);
		SetOP Set;

		typedef cause::pair<tick_time> (*NextClockOP)(
		    timer_store* x);
		NextClockOP NextClock;

		typedef cause::type (*PostOP)(
		    timer_store* x, tick_time clock);
		PostOP Post;
	};

	template<class X> static bool call_on_timer_store_Set(
	    timer_store* x, timer_message* msg) {
		return static_cast<X*>(x)->on_timer_store_Set(msg);
	}

	template<class X> static cause::pair<tick_time>
	call_on_timer_store_NextClock(timer_store* x) {
		return static_cast<X*>(x)->on_timer_store_NextClock();
	}

	template<class X> static cause::type call_on_timer_store_Post(
	    timer_store* x, tick_time clock) {
		return static_cast<X*>(x)->on_timer_store_Post(clock);
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
	void set_store(timer_store* tq);
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

	timer_store* store;

public:
	void dump(output_buffer& ob);
};

cause::type get_jiffy_tick(tick_time* tick);


#endif  // include guard

