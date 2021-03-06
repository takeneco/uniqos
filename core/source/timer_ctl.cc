/// @file   timer_ctl.cc
/// @brief  Timer implementation.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2014 KATO Takeshi
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

#include <core/timer_ctl.hh>

#include <core/clock_src.hh>
#include <core/cpu_node.hh>
#include <core/global_vars.hh>
#include <core/mempool.hh>
#include <core/timer.hh>
#include <core/log.hh>
#include <core/thread.hh>
#include <util/bitops.hh>


cause::t hpet_setup(clock_source** clksrc);

namespace {

cause::t detect_clock_src(clock_source** clksrc)
{
	cause::t r;

#if CONFIG_HPET
	r = hpet_setup(clksrc);
	if (is_ok(r))
		return r;

#endif  // CONFIG_HPET

	log()("Clock source not found.")();

	return cause::FAIL;
}

void _on_timer_message(message* msg)
{
	auto _msg = static_cast<message_with<timer_ctl*>*>(msg);
	_msg->data->on_timer_message();
}

}  // namespace

void timer_store::operations::init()
{
	Set = 0;
	NextClock = 0;
}

// time_ctl

timer_ctl::timer_ctl()
{
	timer_msg.handler = _on_timer_message;
	timer_msg.data = this;
}

void timer_ctl::set_clock_source(clock_source* cs)
{
	clk_src = cs;

#	warning error check omitted.
	auto r = clk_src->update_clock();
}

void timer_ctl::set_store(timer_store* tq)
{
	store = tq;
}

cause::t timer_ctl::get_jiffy_tick(tick_time* tick)
{
#	warning error check omitted.
	auto r = clk_src->update_clock();

	*tick = r.get_value();

	return cause::OK;
}

cause::t timer_ctl::set_timer(timer_message* msg)
{
	// 現在時刻
#	warning error check omitted.
	auto now_clk = clk_src->update_clock();

	// どれだけ待つか
#	warning error check omitted.
	auto delay_clk = clk_src->nanosec_to_clock(msg->nanosec_delay);

	// いつまで待つか
	tick_time exp_clk = now_clk.value() + delay_clk.value();

	msg->expires_clock = exp_clk;

	lock.lock();

	// TODO:ここでOUTOFRANGEが帰らないようにする
	const cause::t r = _set_timer(msg, now_clk.value());

	lock.unlock();

	return r;
}

void timer_ctl::on_timer_message()
{
	lock.lock();

#	warning error check omitted.
	clk_src->update_clock();

	for (;;) {
		tick_time now_clock = clk_src->get_latest_clock(); 

		store->post(now_clock);

		//TODO:すでにタイマー設定済みの場合は、再設定の動作にする

		auto next_clock = store->next_clock();
		if (is_ok(next_clock)) {
			const cause::t r =
			    clk_src->set_timer(next_clock.value(), &timer_msg);
			if (r == cause::OUTOFRANGE)
				continue;
		}

		break;
	}

	lock.unlock();
}

/// @brief 現在時刻を now_clock と仮定してタイマをセットする。
//
/// msg->ticks ではなく msg->expires_clock をタイマの時刻とする。
/// ロックしない。
cause::t timer_ctl::_set_timer(timer_message* msg, tick_time now_clock)
{
	if (store->set(msg)) {
		//TODO:すでにタイマー設定済みの場合は、再設定の動作にする

		return clk_src->set_timer(msg->expires_clock, &timer_msg);
	}

	return cause::OK;
}

void timer_ctl::dump(output_buffer& ob)
{
}

#include <core/timer_liner_q.hh>

cause::t timer_setup()
{
	clock_source* clksrc;
	cause::t r = detect_clock_src(&clksrc);
	if (is_fail(r))
		return r;

	timer_liner_store::setup();

	timer_store* liner_q =
	    new (mem_alloc(sizeof (timer_liner_store))) timer_liner_store;
	if (!liner_q)
		return cause::NOMEM;

	timer_ctl* tc = new (mem_alloc(sizeof (timer_ctl))) timer_ctl;
	if (!tc)
		return cause::NOMEM;

	tc->set_clock_source(clksrc);
	tc->set_store(liner_q);

	global_vars::core.timer_ctl_obj = tc;

	return cause::OK;
}

cause::t get_jiffy_tick(tick_time* tick)
{
	return global_vars::core.timer_ctl_obj->get_jiffy_tick(tick);
}

cause::t timer_set(timer_message* m)
{
	return global_vars::core.timer_ctl_obj->set_timer(m);
}


// wakeup_thread_timer_message


wakeup_thread_timer_message::wakeup_thread_timer_message()
{
	message::handler = &timer_handler;
}

void wakeup_thread_timer_message::timer_handler(message* msg)
{
	wakeup_thread_timer_message* m =
	    static_cast<wakeup_thread_timer_message*>(msg);

	m->thr->ready();
}
