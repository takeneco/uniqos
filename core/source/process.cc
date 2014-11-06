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

#include <core/new_ops.hh>
#include <core/process_ctl.hh>
#include <core/thread.hh>


process::process() :
	io_desc_nr(0),
	io_desc_array(nullptr)
{
}

process::~process()
{
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
TODO:
sharedmempoolからメモリの実サイズ情報を取得
}
*/

//TODO: 返した後でio_descが開放されないようにする必要がある。
cause::pair<io_desc*> process::get_io_desc(int iod)
{
	if (iod >= io_desc_nr || iod < 0)
		return null_pair(cause::BADIO);

	if (!io_desc_array[iod])
		return null_pair(cause::BADIO);

	return make_pair(cause::OK, io_desc_array[iod]);
}

cause::t process::set_io_desc(int iod, io_node* target, io_node::offset off)
{
	if (io_desc_nr <= iod)
		return cause::OUTOFRANGE;
	if (io_desc_array[iod] != nullptr) {
		// TODO:close io_desc[iod]
	}

	io_desc* iodesc = new (*get_process_ctl()->io_desc_pool()) io_desc;
	if (!iodesc)
		return cause::NOMEM;

	iodesc->io = target;
	iodesc->off = off;
	io_desc_array[iod] = iodesc;

	return cause::OK;
}

/// @brief 空き iod を探して io_node をセットする。
/// @return iod を返す。
cause::pair<int> process::append_io_desc(io_node* io, io_node::offset off)
{
	int iod;
	for (iod = 0; iod < io_desc_nr; ++iod) {
		if (!io_desc_array[iod])
			break;
	}

	if (iod >= io_desc_nr)
		return zero_pair(cause::MAXIO);

	io_desc* iodesc = new (*get_process_ctl()->io_desc_pool()) io_desc;
	if (!iodesc)
		return zero_pair(cause::NOMEM);

	iodesc->io = io;
	iodesc->off = off;
	io_desc_array[iod] = iodesc;

	return make_pair(cause::OK, iod);
}


process* get_current_process()
{
	thread* t = get_current_thread();

	return t->get_owner_process();
}

#include <core/fs_ctl.hh>

cause::pair<uptr> sys_open(const char* path, u32 flags)
{
	fs_ctl* fsctl = get_fs_ctl();

	auto ion = fsctl->open(path, flags);
	if (is_fail(ion)) {
		return zero_pair(ion.cause());
	}

	process* pr = get_current_process();

	auto iod = pr->append_io_desc(ion.data(), 0);
	if (is_fail(iod)) {
		return zero_pair(iod.cause());
	}

	return cause::pair<uptr>(cause::OK, iod.data());
}

cause::pair<uptr> sys_close(u32 iod)
{
	process* pr = get_current_process();

	auto iodesc = pr->get_io_desc(iod);
	if (is_fail(iodesc))
		return zero_pair(iodesc.cause());

	//io_node::close(iodesc.data()->io);

	cause::t r = pr->set_io_desc(iod, nullptr, 0);
	if (is_fail(r)) {
		return zero_pair(r);
	}

	return zero_pair(cause::OK);
}

cause::pair<uptr> sys_write(int iod, const void* buf, uptr bytes)
{
	auto r = get_current_process()->get_io_desc(iod);
	if (is_fail(r))
		return zero_pair(r.cause());

	io_desc* target = r.data();

	return target->io->write(target->off, buf, bytes);
}

