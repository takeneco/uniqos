/// @file   text_vga.cc
/// @brief  text mode VGA output i/f.
//
// (C) 2011 KATO Takeshi
//

#include "vga.hh"


void text_vga::init(u32 _width, u32 _height, void* _vram)
{
	fops.write = write;
	ops = &fops;

	width = _width;
	height = _height;
	vram = reinterpret_cast<u8*>(_vram);

	xpos = ypos = 0;
}

cause::stype text_vga::write(
    file_interface* self, const void* data, uptr size, uptr offset)
{
	return static_cast<text_vga*>(self)->_write(
	    reinterpret_cast<const u8*>(data),
	    size);
}

cause::stype text_vga::_write(const u8* data, uptr size)
{
	for (uptr i = 0; i < size; ++i) {
		if (data[i] == '\r')
			continue;
		if (data[i] == '\n') {
			xpos = 0;
			++ypos;
			continue;
		}
		if (xpos == width - 1) {
			xpos = 0;
			++ypos;
		}
		putc(data[i]);
	}

	return cause::OK;
}

void text_vga::putc(u8 c)
{
	int offset = (xpos + ypos * width) * 2;
	vram[offset] = c;
	vram[offset + 1] = 0xf;

	++xpos;
}

