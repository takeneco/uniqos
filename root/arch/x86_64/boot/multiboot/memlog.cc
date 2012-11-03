/// @file   memlog.cc
/// @brief  On memory log.
/// @detail カーネルへログを渡するためにメモリ上にログを書く。

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "misc.hh"


namespace {

enum {
	MAX_SIZE = 8192,
	HEAPSLOTM = SLOTM_BOOTHEAP | SLOTM_NORMAL,
};

io_node::operations memlog_ops;

}  // namespace

cause::type memlog_file::setup()
{
	memlog_ops.init();

	memlog_ops.seek  = call_on_io_node_seek<memlog_file>;
	memlog_ops.read  = call_on_io_node_read<memlog_file>;
	memlog_ops.write = call_on_io_node_write<memlog_file>;

	return cause::OK;
}

cause::type memlog_file::open()
{
	void* p = get_alloc()->alloc(HEAPSLOTM, MAX_SIZE, 1, true);

	buf = reinterpret_cast<u8*>(p);
	if (!buf)
		return cause::NOMEM;

	size = 0;

	io_node::ops = &memlog_ops;

	return cause::OK;
}

cause::type memlog_file::close()
{
	return cause::OK;
}

cause::type memlog_file::on_io_node_seek(
    seek_whence whence, offset rel_off, offset* abs_off)
{
	return io_node::usual_seek(MAX_SIZE - 1, whence, rel_off, abs_off);
}

cause::type memlog_file::on_io_node_read(offset* off, int iov_cnt, iovec* iov)
{
	if (!buf)
		return cause::INVALID_OBJECT;

	offset _off = *off;
	iovec_iterator itr(iov_cnt, iov);
	while (_off < size) {
		u8* c = itr.next_u8();
		if (!c)
			break;

		*c = buf[_off++];
	}

	*off = _off;

	return cause::OK;
}

cause::type memlog_file::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	if (!buf)
		return cause::INVALID_OBJECT;

	offset _off = *off;
	iovec_iterator itr(iov, iov_cnt);
	while (_off < MAX_SIZE) {
		const u8* c = itr.next_u8();
		if (!c)
			break;

		buf[_off++] = *c;
	}

	*off = _off;

	size = max(_off, size);

	return itr.is_end() ? cause::OK : cause::FAIL;
}

