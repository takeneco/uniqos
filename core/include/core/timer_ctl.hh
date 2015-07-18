/// @file  core/timer_ctl.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

/// タイマデバイスが刻む時刻を clock と呼び、
/// カーネル内時刻を tick と呼ぶことにする。

#ifndef CORE_TIMER_CTL_HH_
#define CORE_TIMER_CTL_HH_

#include <core/clock_src.hh>
#include <config.h>
#include <core/timer.hh>
#include <core/spinlock.hh>


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

cause::t get_jiffy_tick(tick_time* tick);


#endif  // include guard

