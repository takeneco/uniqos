/// @file   syscall_entry.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2013-2014 KATO Takeshi
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

#include <core/basic.hh>
#include <core/cpu_node.hh>
#include <core/log.hh>
#include <core/timer.hh>


extern "C" cause::pair<uptr> syscall_entry(const ucpu* data)
{
	log()('#');

	if (data[0] == 100) { // timer
		wakeup_thread_timer_message wttm;
		cpu_node* cn = get_cpu_node();
		thread_queue& tq = cn->get_thread_ctl();
		wttm.thr = tq.get_running_thread();
		wttm.nanosec_delay = 100000000;
		timer_set(&wttm);
		sleep_current_thread();
	}

	return cause::pair<uptr>(cause::OK, 0);
}

