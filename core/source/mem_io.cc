/// @file   mem_io.cc
/// @brief  mem_io class implements.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2014 KATO Takeshi
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

#include <mem_io.hh>
#include <setup.hh>
#include <core/string.hh>


//TODO
io_node::interfaces mem_io_node_ifs;
io_node::interfaces ringed_mem_io_node_ifs;

// mem_io

mem_io::mem_io(void* _contents, uptr bytes) :
	io_node(&mem_io_node_ifs),
	contents(static_cast<u8*>(_contents)),
	capacity_bytes(bytes)
{
}

cause::t mem_io::on_io_node_seek(
    seek_whence whence, offset rel_off, offset* abs_off)
{
	return io_node::usual_seek(
	    capacity_bytes - 1, whence, rel_off, abs_off);
}

cause::pair<uptr> mem_io::on_Read(offset off, void* data, uptr bytes)
{
	offset left_bytes = capacity_bytes - off;
	if (left_bytes < 0)
		left_bytes = 0;

	uptr read_bytes = min<uptr>(left_bytes, bytes);

	mem_copy(&contents[off], data, read_bytes);

	return make_pair(cause::OK, read_bytes);
}

cause::t mem_io::on_io_node_read(offset* off, int iov_cnt, iovec* iov)
{
	iovec_iterator iov_i(iov_cnt, iov);

	uptr read_bytes = iov_i.write(capacity_bytes - *off, &contents[*off]);

	*off += read_bytes;

	return cause::OK;
}

cause::pair<uptr> mem_io::on_Write(offset off, const void* data, uptr bytes)
{
	offset left_bytes = capacity_bytes - off;
	if (left_bytes < 0)
		left_bytes = 0;

	uptr write_bytes = min<uptr>(left_bytes, bytes);

	mem_copy(data, &contents[off], write_bytes);

	return make_pair(cause::OK, write_bytes);
}

cause::t mem_io::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	if (*off > capacity_bytes)
		return cause::FAIL;

	iovec_iterator iov_i(iov_cnt, iov);

	uptr write_bytes = iov_i.read(capacity_bytes - *off, &contents[*off]);

	*off += write_bytes;

	return cause::OK;
}

cause::t mem_io::setup()
{
	mem_io_node_ifs.init();
	mem_io_node_ifs.seek = call_on_io_node_seek<mem_io>;
	mem_io_node_ifs.Read   = call_on_Read<mem_io>;
	mem_io_node_ifs.Write  = call_on_Write<mem_io>;
	mem_io_node_ifs.read = call_on_io_node_read<mem_io>;
	mem_io_node_ifs.write  = call_on_io_node_write<mem_io>;

	return cause::OK;
}


// ringed_mem_io

ringed_mem_io::ringed_mem_io(void* _contents, uptr bytes) :
	io_node(&ringed_mem_io_node_ifs),
	contents(static_cast<u8*>(_contents)),
	capacity_bytes(bytes)
{
}

cause::pair<uptr> ringed_mem_io::on_Read(offset off, void* data, uptr bytes)
{
	u8* _data = static_cast<u8*>(data);

	uptr read_bytes = 0;

	while (bytes > 0) {
		const uoffset noff = offset_normalize(off);

		uptr r_bytes = min<uptr>(bytes, capacity_bytes - noff);

		mem_copy(&contents[noff], _data, r_bytes);

		off += r_bytes;
		_data += r_bytes;
		bytes -= r_bytes;
		read_bytes += r_bytes;
	}

	return make_pair(cause::OK, read_bytes);
}

cause::t ringed_mem_io::on_io_node_read(
    offset* off, int iov_cnt, iovec* iov)
{
	iovec_iterator iov_i(iov_cnt, iov);

	while (!iov_i.is_end()) {
		const uptr noff = offset_normalize(*off);

		uptr read_bytes =
		    iov_i.write(capacity_bytes - noff, &contents[noff]);

		*off += read_bytes;
	}

	return cause::OK;
}

cause::pair<uptr> ringed_mem_io::on_Write(
    offset off, const void* data, uptr bytes)
{
	const u8* _data = static_cast<const u8*>(data);

	uptr write_bytes = 0;

	while (bytes > 0) {
		const uoffset noff = offset_normalize(off);

		uptr w_bytes = min<uptr>(bytes, capacity_bytes - noff);

		mem_copy(_data, &contents[noff], w_bytes);

		off += w_bytes;
		_data += w_bytes;
		bytes -= w_bytes;
		write_bytes += w_bytes;
	}

	return make_pair(cause::OK, write_bytes);
}

cause::t ringed_mem_io::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	iovec_iterator iov_i(iov_cnt, iov);

	while (!iov_i.is_end()) {
		const uptr noff = offset_normalize(*off);

		uptr write_bytes =
		    iov_i.read(capacity_bytes - noff, &contents[noff]);

		*off += write_bytes;
	}

	contents[offset_normalize(*off)] = 0xff;

	return cause::OK;
}

io_node::uoffset ringed_mem_io::offset_normalize(offset off)
{
	off %= capacity_bytes;

	if (off < 0)
		off += capacity_bytes;

	return off;
}

cause::t ringed_mem_io::setup()
{
	ringed_mem_io_node_ifs.init();
	ringed_mem_io_node_ifs.Read   = call_on_Read<ringed_mem_io>;
	ringed_mem_io_node_ifs.Write  = call_on_Write<ringed_mem_io>;
	ringed_mem_io_node_ifs.read  = call_on_io_node_read<ringed_mem_io>;
	ringed_mem_io_node_ifs.write = call_on_io_node_write<ringed_mem_io>;

	return cause::OK;
}


/// @brief  setup mem_io and ringed_mem_io.
cause::t mem_io_setup()
{
	cause::t r = mem_io::setup();
	if (is_fail(r))
		return r;

	r = ringed_mem_io::setup();
	if (is_fail(r))
		return r;

	return cause::OK;
}

