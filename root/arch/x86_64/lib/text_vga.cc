/// @file   text_vga.cc
/// @brief  text mode VGA output i/f.
//
// (C) 2011 KATO Takeshi
//

#include "vga.hh"


void text_vga::init(u32 _width, u32 _height, void* _vram)
{
	fops.write = op_write;
	ops = &fops;

	width = _width;
	height = _height;
	vram = reinterpret_cast<u8*>(_vram);

	xpos = ypos = 0;
}

cause::stype text_vga::op_write(
    file* x, const iovec* iov, int iov_cnt, uptr* bytes)
{
	return static_cast<text_vga*>(x)->write(iov, iov_cnt, bytes);
}

cause::stype text_vga::write(const iovec* iov, int iov_cnt, uptr* bytes)
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

	*bytes = total;

	return cause::OK;
}

void text_vga::putc(u8 c)
{
	int offset = (xpos + ypos * width) * 2;
	vram[offset] = c;
	vram[offset + 1] = 0xf;

	++xpos;
}

