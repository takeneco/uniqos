/// @file   kernel/mem_io.cc
/// @brief  mem_io class implements.

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <mem_io.hh>

#include <setup.hh>


//TODO
io_node::operations mem_io_node_ops;

mem_io::mem_io(uptr bytes, void* _contents) :
	io_node(&mem_io_node_ops),
	contents(static_cast<u8*>(_contents)),
	capacity_bytes(bytes)
{
}

cause::type mem_io::on_io_node_seek(
    seek_whence whence, offset rel_off, offset* abs_off)
{
	return io_node::usual_seek(
	    capacity_bytes - 1, whence, rel_off, abs_off);
}

cause::type mem_io::on_io_node_read(offset* off, int iov_cnt, iovec* iov)
{
	iovec_iterator iov_i(iov_cnt, iov);

	uptr read_bytes = iov_i.write(capacity_bytes - *off, &contents[*off]);

	*off += read_bytes;

	return cause::OK;
}

cause::type mem_io::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	if (*off > capacity_bytes)
		return cause::FAIL;

	iovec_iterator iov_i(iov_cnt, iov);

	uptr write_bytes = iov_i.read(capacity_bytes - *off, &contents[*off]);

	*off += write_bytes;

	return cause::OK;
}

cause::type mem_io::setup()
{
	mem_io_node_ops.seek = call_on_io_node_seek<mem_io>;
	mem_io_node_ops.read = call_on_io_node_read<mem_io>;
	mem_io_node_ops.write  = call_on_io_node_write<mem_io>;

	return cause::OK;
}

cause::type mem_io_setup()
{
	return mem_io::setup();
}

