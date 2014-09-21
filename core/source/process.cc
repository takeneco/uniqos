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
#include <core/new_ops.hh>
#include <core/thread.hh>


process::process() :
	io_desc_nr(0),
	io_desc_array(nullptr)
{
}

process::~process()
{
}

//TODO: 返した後でio_descが開放されないようにする必要がある。
cause::pair<io_desc*> process::get_io_desc(int i)
{
	if (i >= io_desc_nr || i < 0)
		return null_pair(cause::BADIO);

	if (!io_desc_array[i])
		return null_pair(cause::BADIO);

	return make_pair(cause::OK, io_desc_array[i]);
}

cause::t process::setup(thread* entry_thread, int iod_nr)
{
	io_desc_nr = iod_nr;
	id = entry_thread->get_thread_id();

	io_desc_array = new (generic_mem()) io_desc*[iod_nr];
	if (!io_desc_array)
		return cause::NOMEM;

	for (int i = 0; i < iod_nr; ++i)
		io_desc_array[i] = nullptr;

	entry_thread->set_owner_process(this);

	child_thread_chain.push_back(entry_thread);

	return cause::OK;
}
/*
cause::t process::set_io_desc_nr(int nr)
{
sharedmempoolからメモリの実サイズ情報を取得
}
*/
cause::t process::set_io_desc(int iod, io_node* target)
{
	if (io_desc_nr <= iod)
		return cause::OUTOFRANGE;
	if (io_desc_array[iod] != nullptr) {
		// TODO:close io_desc[iod]
	}

	//TODO:
	//io_desc_array[iod] = target;

	return cause::OK;
}

process* get_current_process()
{
	thread* t = get_current_thread();

	return t->get_owner_process();
}


cause::pair<uptr> sys_write(int iod, const void* buf, uptr bytes)
{
	auto r = get_current_process()->get_io_desc(iod);
	if (is_fail(r))
		return zero_pair(r.get_cause());

	io_desc* target = r.get_data();
	return target->io->write(target->off, buf, bytes);
}

