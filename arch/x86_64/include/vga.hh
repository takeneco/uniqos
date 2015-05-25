/// @file   vga.hh
/// @brief  declare of text_vga (and graph_vga).
//
// (C) 2011-2015 KATO Takeshi
//

#ifndef X86_64_ARCH_VGA_HH_
#define X86_64_ARCH_VGA_HH_

#include <core/io_node.hh>


class text_vga : public io_node
{
	io_node::interfaces _ifs;

	u32 width;
	u32 height;
	u8* vram;

	u32 xpos;
	u32 ypos;

public:
	text_vga() {}
	void init(u32 _width, u32 _height, void* _vram);

	cause::pair<uptr> on_Write(offset off, const void* data, uptr bytes);
	cause::t on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	void putc(u8 c);
};


#endif  // include guard

