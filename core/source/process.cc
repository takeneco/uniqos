/// @file   process.cc
/// @brief  process class implementation.

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

#include <core/process.hh>

#include <core/io_node.hh>
#include <new_ops.hh>
#include <thread.hh>


process::process()
{
}

process::~process()
{
}

cause::t process::init(thread* entry_thread, int iod_nr)
{
	io_desc_nr = iod_nr;
	id = entry_thread->get_thread_id();

	io_desc = new (shared_mem()) io_node*[iod_nr];
	if (!io_desc)
		return cause::NOMEM;

	for (int i = 0; i < iod_nr; ++i)
		io_desc[i] = nullptr;

	return cause::OK;
}

cause::t process::set_io_desc(int iod, io_node* target)
{
	if (io_desc_nr <= iod)
		return cause::OUTOFRANGE;
	if (io_desc[iod] != nullptr) {
		// TODO:close io_desc[iod]
	}

	io_desc[iod] = target;

	return cause::OK;
}

process* get_current_process()
{
}


cause::pair<uptr> sys_write(int iod, const void* buf, uptr bytes)
{
}

