/// @file   kernel/io_node.cc
/// @brief  io_node class implements.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2014 KATO Takeshi
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

#include <core/io_node.hh>

#include <util/string.hh>


void iovec_iterator::normalize()
{
	while (iov_index < iov_cnt) {
		if (base_offset < iov[iov_index].bytes)
			break;
		iov_index++;
		base_offset = 0;
	}
}

// Returns 1 char ptr, and increment.
// @return  Returns next u8 ptr.
// @return  If no buffer, returns null.
u8* iovec_iterator::next_u8()
{
	if (iov_index >= iov_cnt)
		return 0;

	u8* const base = reinterpret_cast<u8*>(iov[iov_index].base);
	u8* const r = &base[base_offset];

	++base_offset;
	normalize();

	return r;
}

uptr iovec_iterator::left_bytes() const
{
	uptr total = iov[iov_index].bytes - base_offset;

	for (uptr i = iov_index + 1; i < iov_cnt; ++i)
		total += iov[i].bytes;

	return total;
}

/// @return  The number of bytes written.
uptr iovec_iterator::write(uptr bytes, const void* src)
{
	const u8* p = reinterpret_cast<const u8*>(src);

	uptr total = 0;

	while (bytes > 0 && !is_end()) {
		const uptr size =
		    min(iov[iov_index].bytes - base_offset, bytes);
		u8* base = reinterpret_cast<u8*>(iov[iov_index].base);
		mem_copy(p, &base[base_offset], size);

		p += size;
		bytes -= size;
		base_offset += size;
		total += size;

		normalize();
	}

	return total;
}

/// @return  The number of bytes read.
uptr iovec_iterator::read(uptr bytes, void* dest)
{
	u8* p = reinterpret_cast<u8*>(dest);

	uptr total = 0;

	while (bytes > 0 && !is_end()) {
		const uptr size =
		    min(iov[iov_index].bytes - base_offset, bytes);
		const u8* base = reinterpret_cast<u8*>(iov[iov_index].base);
		mem_copy(&base[base_offset], p, size);

		p += size;
		bytes -= size;
		base_offset += size;
		total += size;

		normalize();
	}

	return total;
}

// io_node::interfaces

void io_node::interfaces::init()
{
	seek = io_node::nofunc_Seek;
	read = io_node::nofunc_ReadV;
	write = io_node::nofunc_WriteV;
	Close        = io_node::nofunc_Close;
	Read         = io_node::nofunc_Read;
	Write        = io_node::nofunc_Write;
	GetDirEntry  = io_node::nofunc_GetDirEntry;
}


// io_node

/// seek可能な範囲を [0, upper_limit] と仮定して seek 相当の結果を返す。
cause::t io_node::usual_seek(
    offset upper_limit,
    seek_whence whence,
    offset rel_off,
    offset* abs_off)
{
	offset abs;

	switch (whence) {
	case BEG:
		abs = 0;
		break;
	case ADD:
		abs = *abs_off;
		break;
	case END:
		abs = upper_limit;
		break;
	default:
		return cause::BADARG;
	}

	if (rel_off >= 0) {
		if (static_cast<uoffset>(abs + rel_off) <= OFFSET_MAX) {
			offset tmp = abs + rel_off;
			if (tmp > upper_limit)
				return cause::OUTOFRANGE;
		} else // overflow
			return cause::OUTOFRANGE;
	} else {
		offset tmp = abs + rel_off;
		if (tmp >= 0)
			abs = tmp;
		else  // underflow
			return cause::OUTOFRANGE;
	}

	*abs_off = abs;

	return cause::OK;
}

