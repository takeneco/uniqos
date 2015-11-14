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

#include <core/process.hh>

cause::t sys_mount(
    const char* source,
    const char* target,
    const char* type,
    u64 flags,
    const void* data);
cause::pair<uptr> sys_read(
    int iod,
    void* buf,
    uptr bytes);
cause::pair<uptr> sys_write(
    int iod,
    const void* buf,
    uptr bytes);
cause::pair<uptr> sys_open(
    const char* path,
    u32 flags);
cause::pair<uptr> sys_close(
    u32 iod);
cause::t sys_mkdir(
    const char* path);

cause::pair<uptr> test_string(const char* str)
{
	uptr i;
	for (i = 0; str[i]; ++i)
		;

	return make_pair(cause::OK, i);
}

extern "C" cause::pair<uptr> syscall_entry(const ucpu* data)
{
	cause::pair<uptr> ret;
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
	if (data[0] == 100) { // timer
		wakeup_thread_timer_message wttm;
		cpu_node* cn = get_cpu_node();
		wttm.thr = get_current_thread();
		wttm.nanosec_delay = 1000000000L;
		timer_set(&wttm);
		sleep_current_thread();
		log()("s");
	}
	else if (data[0] == 101) { // mount
		const char* _src = (const char*)data[4];
		const char* _tgt = (const char*)data[3];
		const char* _typ = (const char*)data[2];
		u32 _flg = (u32)data[7];
		const void* _data = (const void*)data[5];
		uptr val = (uptr)data[6];

		cause::t r = sys_mount(_src, _tgt, _typ, _flg, _data);
		ret.set_cause(r);
		ret.set_data(0);
	}
	else if (data[0] == 102) {
		ret = sys_read(data[4], (void*)data[3], data[2]);
	}
	else if (data[0] == 103) { // write
		ret = sys_write(data[4], (const void*)data[3], data[2]);
	}
	else if (data[0] == 104) { // open
		ret = sys_open((const char*)data[4], data[3]);
	}
	else if (data[0] == 105) { // close
		ret = sys_close(data[4]);
	}
	else if (data[0] == 106) { // mkdir
		cause::t r = sys_mkdir((const char*)data[4]);
		ret.set_cause(r);
		ret.set_data(0);
	}

	return ret;
}

