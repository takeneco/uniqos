/// @file   syscall_entry.cc
/// @brief  System call entry point.

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

#include <core/cpu_node.hh>


namespace uniqos {

cause::pair<uptr> core_syscall_entry(const ucpu* data);

}  // namespace uniqos

#include <core/timer.hh>

extern "C" cause::pair<ucpu> syscall_entry(const ucpu* data)
{
    cause::pair<uptr> ret;

    /* data[0] : %rax (system call number)
     * data[1] : %rdi (1st param)
     * data[2] : %rsi (2nd param)
     * data[3] : %rdx (3rd param)
     * data[4] : %r10 (4th param)
     * data[5] : %r8  (5th param)
     * data[6] : %r9  (6th param)
     * data[7] : %rip
     * data[8] : %eflags
     */
    if (data[0] < 100) {
        ret = uniqos::core_syscall_entry(data);
    }
    else if (data[0] == 100) { // timer
        wakeup_thread_timer_message wttm;
        wttm.thr = get_current_thread();
        wttm.nanosec_delay = 1000000000L;
        timer_set(&wttm);
        sleep_current_thread();
    }

    return ret;
}

