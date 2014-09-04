// @file   thread.cc
// @brief  thread class implements.

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

#include <arch/thread_ctl.hh>
#include <core/cpu_node.hh>


thread::thread(thread_id tid) :
	owner_cpu(nullptr),
	id(tid),
	state(SLEEPING),
	anti_sleep(false)
{
}

void thread::ready()
{
	owner_cpu->ready_thread(this);
}

void sleep_current_thread()
{
	arch::sleep_current_thread();
}

thread* get_current_thread()
{
	return arch::get_current_thread();
}

