/// @file   memlog.cc
/// @brief  On memory log.
/// @detail カーネルへログを渡するためにメモリ上にログを書く。

//  Uniqos  --  Unique Operating System
//  (C) 2011 KATO Takeshi
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

file::operations memlog_ops;

} // namespace

cause::stype memlog_file::setup()
{
	memlog_ops.seek  = op_seek;
	memlog_ops.read  = op_read;
	memlog_ops.write = op_write;

	return cause::OK;
}

cause::stype memlog_file::open()
{
	void* p = get_alloc()->alloc(HEAPSLOTM, MAX_SIZE, 1, true);

	buf = reinterpret_cast<u8*>(p);
	if (!buf)
		return cause::NOMEM;

	size = current = 0;

	file::ops = &memlog_ops;

	return cause::OK;
}

cause::stype memlog_file::close()
{
	return cause::OK;
}

cause::stype memlog_file::op_seek(file* x, s64 offset, int whence)
{
	return static_cast<memlog_file*>(x)->seek(offset, whence);
}

cause::stype memlog_file::seek(s64 offset, int whence)
{
	s64 base;
	switch (whence) {
	case file::BEG:
		base = 0;
		break;
	case file::CUR:
		base = current;
		break;
	case file::END:
		base = MAX_SIZE - 1;
		break;
	default:
		return cause::INVALID_PARAMS;
	}

	const s64 new_cur = base + offset;
	if (new_cur < 0 || MAX_SIZE <= new_cur)
		return cause::INVALID_PARAMS;

	current = new_cur;

	return cause::OK;
}

cause::stype memlog_file::op_read(file* x, iovec* iov, int iov_cnt, uptr* bytes)
{
	return static_cast<memlog_file*>(x)->read(iov, iov_cnt, bytes);
}

cause::stype memlog_file::read(iovec* iov, int iov_cnt, uptr* bytes)
{
	if (!buf)
		return cause::INVALID_OBJECT;

	uptr total = 0;
	iovec_iterator itr(iov, iov_cnt);
	while (current < size) {
		u8* c = itr.next_u8();
		if (!c)
			break;

		*c = buf[current++];
		++total;
	}

	*bytes = total;

	return cause::OK;
}

cause::stype memlog_file::op_write(
    file* x, const iovec* iov, int iov_cnt, uptr* bytes)
{
	return static_cast<memlog_file*>(x)->write(iov, iov_cnt, bytes);
}

cause::stype memlog_file::write(const iovec* iov, int iov_cnt, uptr* bytes)
{
	if (!buf)
		return cause::INVALID_OBJECT;

	uptr total = 0;
	iovec_iterator itr(iov, iov_cnt);
	while (current < MAX_SIZE) {
		const u8* c = itr.next_u8();
		if (!c)
			break;

		buf[current++] = *c;
		++total;
	}

	*bytes = total;

	size = max(current, size);

	return itr.is_end() ? cause::OK : cause::FAIL;
}

