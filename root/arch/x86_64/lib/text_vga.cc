/// @file   text_vga.cc
/// @brief  text mode VGA output i/f.
//
// (C) 2011-2012 KATO Takeshi
//

#include <vga.hh>


void text_vga::init(u32 _width, u32 _height, void* _vram)
{
	fops.write = call_on_write<text_vga>;
	ops = &fops;

	width = _width;
	height = _height;
	vram = reinterpret_cast<u8*>(_vram);

	xpos = ypos = 0;
}

cause::type text_vga::on_write(offset* off, int iov_cnt, const iovec* iov)
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

