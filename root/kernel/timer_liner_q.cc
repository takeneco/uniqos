/// @file   timer_lner_q.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <timer_liner_q.hh>

#include <cpu_node.hh>


namespace {

timer_queue::operations timer_liner_q_ops;

}  // namespace


cause::type timer_liner_queue::setup()
{
	timer_queue::operations& ops = timer_liner_q_ops;

	ops.init();

	ops.Insert =
	    timer_queue::call_on_timer_queue_Insert<timer_liner_queue>;
	ops.NextClock =
	    timer_queue::call_on_timer_queue_NextClock<timer_liner_queue>;
	ops.Post =
	    timer_queue::call_on_timer_queue_Post<timer_liner_queue>;

	return cause::OK;
}

timer_liner_queue::timer_liner_queue()
{
	ops = &timer_liner_q_ops;
}

/// @retval true メッセージがキューの先頭に入った。
/// @retval false メッセージがキューの先頭以外の場所に入った。
//
/// 戻り値が true の場合は、タイマの再設定が必要。
bool timer_liner_queue::on_timer_queue_Insert(timer_message* new_msg)
{
	const tick_time clk = new_msg->expires_clock;

	auto msg = msg_chain.front();

	if (!msg || clk < msg->expires_clock) {
		msg_chain.push_front(new_msg);
		return true;
	}
	for (;;) {
		auto msg2 = msg_chain.next(msg);
		if (!msg2 || clk < msg2->expires_clock)
			break;
		msg = msg2;
	}

	msg_chain.insert_next(msg, new_msg);
	return false;
}

cause::pair<tick_time> timer_liner_queue::on_timer_queue_NextClock()
{
	auto next_msg = msg_chain.front();

	if (next_msg) {
		return cause::pair<tick_time>(
		    cause::OK, next_msg->expires_clock);
	} else {
		return cause::pair<tick_time>(cause::FAIL, 0);
	}
}

cause::type timer_liner_queue::on_timer_queue_Post(tick_time clock)
{
	cpu_node* cpu = get_cpu_node();

	for (auto msg = msg_chain.front(); msg; msg = msg_chain.next(msg)) {
		if (msg->expires_clock < clock)
			cpu->post_soft_message(msg);
	}

	return cause::OK;
}

