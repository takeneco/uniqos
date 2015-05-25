/// @file   text_vga.cc
/// @brief  text mode VGA output interface.

//  Uniqos  --  Unique Operating System
//  (C) 2011-2015 KATO Takeshi
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

#include <arch/vga.hh>


void text_vga::init(u32 _width, u32 _height, void* _vram)
{
	_ifs.init();

	_ifs.Write  = call_on_Write<text_vga>;
	_ifs.write = call_on_io_node_write<text_vga>;
	ifs = &_ifs;

	width = _width;
	height = _height;
	vram = reinterpret_cast<u8*>(_vram);

	xpos = ypos = 0;
}

cause::pair<uptr> text_vga::on_Write(
    offset /*off*/, const void* data, uptr bytes)
{
	const u8* _data = static_cast<const u8*>(data);

	uptr wrote_bytes = 0;

	for (uptr i = 0; i < bytes; ++i, ++wrote_bytes) {
		u8 c = *_data++;
		if (c == '\r')
			continue;
		if (c == '\n') {
			xpos = 0;
			++ypos;
			continue;
		}
		if (xpos >= width) {
			xpos = 0;
			++ypos;
		}
		putc(c);
	}

	return make_pair(cause::OK, wrote_bytes);
}

cause::t text_vga::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	uptr total = 0;

	iovec_iterator itr(iov, iov_cnt);
	for (;; ++total) {
		const u8 *c = itr.next_u8();
		if (!c)
			break;
		if (*c == '\r')
			continue;
		if (*c == '\n') {
			xpos = 0;
			++ypos;
			continue;
		}
		if (xpos >= width) {
			xpos = 0;
			++ypos;
		}
		putc(*c);
	}

	*off += total;

	return cause::OK;
}

void text_vga::putc(u8 c)
{
	int offset = (xpos + ypos * width) * 2;
	vram[offset] = c;
	vram[offset + 1] = 0xf;

	++xpos;
}

