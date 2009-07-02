/**
 * @file    arch/x86/boot/phase4/phase4.hpp
 * @version 0.0.2
 * @date    2009-07-02
 * @author  Kato.T
 *
 * phase4 で使用する機能の宣言。
 */
// (C) Kato.T 2009

#ifndef _ARCH_X86_BOOT_PHASE4_PHASE4_HPP
#define _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

#include "btypes.hpp"

/**
 * テキストモード画面出力
 */
class console
{
	int   width;
	int   height;
	char* vram;
	int   cur_row;
	int   cur_col;
public:
	void init(int w, int h, _u32 vram_addr);
	void set_cur(int row, int col) { cur_row = row; cur_col = col; }
	console* putc(char ch);
	console* puts(const char* str);
	console* putu32(_u32 n);
	console* putu32x(_u32 n);
	console* putu64x(_u64 n);
};

#endif  // _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

