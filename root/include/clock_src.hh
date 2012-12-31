/// @file  clock_src.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_CLOCK_SRC_HH_
#define INCLUDE_CLOCK_SRC_HH_

#include <basic.hh>


typedef cycle_scalar<u64> tick_time;

class clock_source
{
	DISALLOW_COPY_AND_ASSIGN(clock_source);

protected:
	struct operations
	{
		void init();

		typedef cause::type (*get_tick_op)(
		    clock_source* x, tick_time* tick);
		get_tick_op get_tick;
	};

	template<class T> static cause::type call_on_clock_source_get_tick(
	    clock_source* x, tick_time* tick) {
		return static_cast<T*>(x)->
		    on_clock_source_get_tick(tick);
	}
	static cause::type nofunc_clock_source_get_tick(
	    clock_source*, tick_time*) {
		return cause::NOFUNC;
	}

protected:
	clock_source() {}
	clock_source(const operations* _ops) : ops(_ops) {}

public:
	cause::type get_tick(tick_time* tick) {
		return ops->get_tick(this, tick);
	}

protected:
	const operations* ops;
};


#endif  // include guard

