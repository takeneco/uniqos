/* FILE : arch/x86/boot/phase4/phase4.hpp
 * VER  : 0.0.1
 * LAST : 2009-06-31
 * (C) Kato.T 2009
 */

#ifndef _ARCH_X86_BOOT_PHASE4_PHASE4_HPP
#define _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

#include "btypes.hpp"

class console
{
	int   width;
	int   height;
	char* vram;
	int   curx;
	int   cury;
public:
	void init(int w, int h, _u32 vram_addr);
	void putc(char ch);
};

#endif  // _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

