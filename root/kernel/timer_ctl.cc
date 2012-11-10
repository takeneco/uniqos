/// @file   timer_ctl.cc
/// @brief  Timer implementation.

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <timer_ctl.hh>

#include <global_vars.hh>
#include <mempool.hh>
#include <new_ops.hh>


class timer_ctl::timer_queue
{
	DISALLOW_COPY_AND_ASSIGN(timer_queue);

public:
	timer_queue() {}

private:
	chain<timer_message, &timer_message::chain_hook> message_chain;
};


timer_ctl::timer_ctl()
{
}


cause::type timer_ctl::setup_cpu()
{
	cpu_id cpuid = arch::get_cpu_id();

	queue[cpuid] = new (mem_alloc(sizeof (timer_ctl))) timer_queue;
	if (!queue[cpuid])
		return cause::NOMEM;

	return cause::OK;
}

/// timer_setup_cpu() の前に１回だけ呼び出す必要がある。
cause::type timer_setup()
{
	timer_ctl* tc = new (mem_alloc(sizeof (timer_ctl))) timer_ctl;
	if (!tc)
		return cause::NOMEM;

	global_vars::core.timer_ctl_obj = tc;

	return cause::OK;
}

/// 各CPUがタイマを使う前に呼び出す必要がある。
cause::type timer_setup_cpu()
{
	return global_vars::core.timer_ctl_obj->setup_cpu();
}

