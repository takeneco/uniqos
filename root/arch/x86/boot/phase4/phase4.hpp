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

#include <cstddef>
#include "btypes.hpp"


/**
 * 出力専用端末の基本クラス。
 */
class outterm
{

};

/**
 * 出力用シリアル端末。
 */
class com_term : public outterm
{
	_u16 base_port;

	/// 出力バッファ
	char buf[256];
	// buf 中の未出力データの先頭と終端
	int data_head;
	int data_tail;

public:
	void init(_u16 port);
	void putc(char ch);

public:
	enum {
		COM1_BASE = 0x03f8,
		COM2_BASE = 0x02f8,
	};
};


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

	void roll(unsigned int n);
public:
	void init(int w, int h, _u32 vram_addr);
	void set_cur(int row, int col) { cur_row = row; cur_col = col; }
	console* putc(char ch);
	console* puts(const char* str);
	console* putu32(_u32 n);
	console* putu32x(_u32 n);
	console* putu64x(_u64 n);
};


void* memcpy(void* dest, const void* src, std::size_t n);

#endif  // _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

