/// @file   vga.hh
/// @brief  declare of text_vga (and graph_vga).
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_VGA_HH_
#define ARCH_X86_64_INCLUDE_VGA_HH_

#include "base_types.hh"
#include "fileif.hh"


class text_vga : public file_interface
{
	file_ops fops;

	u32 width;
	u32 height;
	u8* vram;

	u32 xpos;
	u32 ypos;

public:
	text_vga() {}
	void init(u32 _width, u32 _height, void* _vram);

private:
	static cause::stype write(
	    file_interface* self, const void* data, uptr size, uptr offset);
	cause::stype _write(const u8* data, uptr size);

	void putc(u8 c);
};


#endif  // include guard
