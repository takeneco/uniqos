/// @file  clock_src.hh
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef CORE_CLOCK_SRC_HH_
#define CORE_CLOCK_SRC_HH_

#include <core/basic.hh>


class message;
typedef u64 tick_int;
typedef cycle_scalar<tick_int> tick_time;

class clock_source
{
	DISALLOW_COPY_AND_ASSIGN(clock_source);

protected:
	struct operations
	{
		void init();

		typedef cause::pair<tick_time> (*UpdateClockOP)(
		    clock_source* x);
		UpdateClockOP UpdateClock;

		typedef cause::type (*SetTimerOP)(
		    clock_source* x, tick_time clock, message* msg);
		SetTimerOP SetTimer;

		typedef cause::pair<u64> (*ClockToNanosecOP)(
		    clock_source* x, u64 clock);
		ClockToNanosecOP ClockToNanosec;

		typedef cause::pair<u64> (*NanosecToClockOP)(
		    clock_source* x, u64 nanosec);
		NanosecToClockOP NanosecToClock;
	};

	template<class T> static cause::pair<tick_time>
	call_on_clock_source_UpdateClock(
	    clock_source* x) {
		return static_cast<T*>(x)->
		    on_clock_source_UpdateClock();
	}

	template<class T> static cause::type
	call_on_clock_source_SetTimer(
	    clock_source* x, tick_time clock, message* msg) {
		return static_cast<T*>(x)->
		    on_clock_source_SetTimer(clock, msg);
	}

	template<class T> static cause::pair<u64>
	call_on_clock_source_ClockToNanosec(
	    clock_source* x, u64 clock) {
		return static_cast<T*>(x)->
		    on_clock_source_ClockToNanosec(clock);
	}

	template<class T> static cause::pair<u64>
	call_on_clock_source_NanosecToClock(
	    clock_source* x, u64 tick) {
		return static_cast<T*>(x)->
		    on_clock_source_NanosecToClock(tick);
	}

protected:
	clock_source() {}
	clock_source(const operations* _ops) : ops(_ops) {}

public:
	tick_time get_latest_clock() const {
		return LatestClock;
	}
	cause::pair<tick_time> update_clock() {
		return ops->UpdateClock(this);
	}
	cause::type set_timer(tick_time clock, message* msg) {
		return ops->SetTimer(this, clock, msg);
	}
	cause::pair<u64> clock_to_nanosec(u64 clock) {
		return ops->ClockToNanosec(this, clock);
	}
	cause::pair<u64> nanosec_to_clock(u64 nanosec) {
		return ops->NanosecToClock(this, nanosec);
	}

protected:
	const operations* ops;

	tick_time LatestClock;
};


#endif  // include guard

