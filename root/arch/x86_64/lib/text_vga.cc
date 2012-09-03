/// @file   text_vga.cc
/// @brief  text mode VGA output interface.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
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

#include <vga.hh>


void text_vga::init(u32 _width, u32 _height, void* _vram)
{
	fops.write = call_on_io_node_write<text_vga>;
	ops = &fops;

	width = _width;
	height = _height;
	vram = reinterpret_cast<u8*>(_vram);

	xpos = ypos = 0;
}

cause::type text_vga::on_io_node_write(
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

