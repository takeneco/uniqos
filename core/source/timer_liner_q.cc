/// @file   timer_liner_q.cc
/// @brief  タイマメッセージ用の線形キュー

//  Uniqos  --  Unique Operating System
//  (C) 2013 KATO Takeshi
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

#include <core/timer_liner_q.hh>

#include <core/cpu_node.hh>


namespace {

timer_store::operations timer_liner_store_ops;

}  // namespace


cause::t timer_liner_store::setup()
{
	timer_store::operations& ops = timer_liner_store_ops;

	ops.init();

	ops.Set =
	    timer_store::call_on_timer_store_Set<timer_liner_store>;
	ops.NextClock =
	    timer_store::call_on_timer_store_NextClock<timer_liner_store>;
	ops.Post =
	    timer_store::call_on_timer_store_Post<timer_liner_store>;

	return cause::OK;
}

timer_liner_store::timer_liner_store()
{
	ops = &timer_liner_store_ops;
}

/// @retval true メッセージがキューの先頭に入った。
/// @retval false メッセージがキューの先頭以外の場所に入った。
//
/// 戻り値が true の場合は、タイマの再設定が必要。
bool timer_liner_store::on_timer_store_Set(timer_message* new_msg)
{
	const tick_time new_msg_clk = new_msg->expires_clock;

	auto msg = msg_chain.front();
	for (; msg; msg = msg_chain.next(msg)) {
		if (new_msg_clk < msg->expires_clock)
			break;
	}

	if (msg) {
		msg_chain.insert_before(msg, new_msg);
	} else {
		msg_chain.push_back(new_msg);
	}

	return msg_chain.front() == new_msg;
}

cause::pair<tick_time> timer_liner_store::on_timer_store_NextClock()
{
	auto next_msg = msg_chain.front();

	if (next_msg) {
		return cause::pair<tick_time>(
		    cause::OK, next_msg->expires_clock);
	} else {
		return cause::pair<tick_time>(cause::FAIL, tick_time(0));
	}
}

cause::t timer_liner_store::on_timer_store_Post(tick_time clock)
{
	auto msg = msg_chain.front();

	while (msg) {
		auto next = msg_chain.next(msg);

		if (msg->expires_clock < clock) {
			msg_chain.remove(msg);
			post_message(msg);
		}

		msg = next;
	}

	return cause::OK;
}

