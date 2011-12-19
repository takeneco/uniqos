/// @file   vga.hh
/// @brief  declare of text_vga (and graph_vga).
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_VGA_HH_
#define ARCH_X86_64_INCLUDE_VGA_HH_

#include "basic_types.hh"
#include "file.hh"


class text_vga : public file
{
	file::operations fops;

	u32 width;
	u32 height;
	u8* vram;

	u32 xpos;
	u32 ypos;

public:
	text_vga() {}
	void init(u32 _width, u32 _height, void* _vram);

private:
	static cause::stype op_write(
	    file* x, const iovec* iov, int iov_cnt, uptr* bytes);
	cause::stype write(const iovec* iov, int iov_cnt, uptr* bytes);

	void putc(u8 c);
};


#endif  // include guard

