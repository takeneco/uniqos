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

#include <clock_src.hh>
#include <global_vars.hh>
#include <log.hh>
#include <mempool.hh>
#include <new_ops.hh>


cause::type hpet_setup(clock_source** clksrc);

namespace {

cause::type detect_clock_src(clock_source** clksrc)
{
	cause::type r;

#if CONFIG_HPET
	r = hpet_setup(clksrc);
	if (is_ok(r))
		return r;

#endif  // CONFIG_HPET

	log()("Clock source not found.")();

	return cause::FAIL;
}

}  // namespace

// time_ctl

timer_ctl::timer_ctl()
{
}

void timer_ctl::set_clock_source(clock_source* cs)
{
	clk_src = cs;
}

cause::type timer_ctl::get_jiffy_tick(tick_time* tick)
{
	tick_time clock;
	clk_src->get_tick(&clock);

	*tick = clock;

	return cause::OK;
}

/// timer_setup_cpu() の前に１回だけ呼び出す必要がある。
cause::type timer_setup()
{
	clock_source* clksrc;
	cause::type r = detect_clock_src(&clksrc);
	if (is_fail(r))
		return r;

	timer_ctl* tc = new (mem_alloc(sizeof (timer_ctl))) timer_ctl;
	if (!tc)
		return cause::NOMEM;

	tc->set_clock_source(clksrc);

	global_vars::core.timer_ctl_obj = tc;

	return cause::OK;
}

cause::type get_jiffy_tick(tick_time* tick)
{
	return global_vars::core.timer_ctl_obj->get_jiffy_tick(tick);
}

