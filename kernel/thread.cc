// @file   thread.cc
// @brief  thread class implements.

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

#include <cpu_node.hh>


#include <native_ops.hh>
thread::thread(
    cpu_node* _owner,
    uptr text,
    uptr param,
    uptr stack,
    uptr stack_size
) :
	owner(_owner),
	rs(text, param, stack, stack_size),
	anti_sleep(false)
{
	// TODO:これはarch依存
	rs.cr3 = native::get_cr3();
}

void thread::ready()
{
	owner->get_thread_ctl().ready(this);
}

