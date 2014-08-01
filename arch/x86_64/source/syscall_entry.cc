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


cause::pair<uptr> test_string(const char* str)
{
	uptr i;
	for (i = 0; str[i]; ++i)
		;

	return make_pair(cause::OK, i);
}

extern "C" cause::pair<uptr> syscall_entry(const ucpu* data)
{
	/* data[0] : %rax
	 * data[1] : %rip
	 * data[2] : %rdx(3rd param)
	 * data[3] : %rsi(2nd param)
	 * data[4] : %rdi(1st param)
	 * data[5] : %r8 (5th param)
	 * data[6] : %r9 (6th param)
	 * data[7] : %r10(4th param)
	 * data[8] : %eflags
	 */
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
	else if (data[0] == 101) { // mount
		const char* _src = (const char*)data[4];
		const char* _tgt = (const char*)data[3];
		const char* _typ = (const char*)data[2];
		u32 _flg = (u32)data[7];
		const void* _data = (const void*)data[5];
		uptr val = (uptr)data[6];

		log()("mount():src=")(_src)(", tgt=")(_tgt)(", typ=")(_typ)
		    (", flg=").x(_flg)(", data=").p(_data)()(", 6=").x(val)();
	}

	return cause::pair<uptr>(cause::OK, 0);
}

