/// @file   vga.hh
/// @brief  declare of text_vga (and graph_vga).
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_VGA_HH_
#define ARCH_X86_64_INCLUDE_VGA_HH_

#include <basic.hh>
#include <io_node.hh>


class text_vga : public io_node
{
	io_node::operations fops;

	u32 width;
	u32 height;
	u8* vram;

	u32 xpos;
	u32 ypos;

public:
	text_vga() {}
	void init(u32 _width, u32 _height, void* _vram);

	cause::type on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	void putc(u8 c);
};


#endif  // include guard

