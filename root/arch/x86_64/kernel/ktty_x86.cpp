/**
 * @file    arch/x86/kernel/ktty_x86.cpp
 * @version 0.0.1
 * @date    2009-08-03
 * @author  Kato.T
 * @brief   カーネルメッセージの出力先。
 */
// (C) Kato.T 2009

#include "btypes.hpp"
#include "ktty.hpp"

#include "../boot/include/boot.h"

void* memcpy(void* dest, const void* src, std::size_t size)
{
	char* d = reinterpret_cast<char*>(dest);
	const char* s = reinterpret_cast<const char*>(src);

	if (dest < src) {
		const int n = size;
		for (int i = 0; i < n; i++) {
			d[i] = s[i];
		}
	}
	else {
		for (int i = size - 1; i >= 0; i--) {
			d[i] = s[i];
		}
	}

	return dest;
}

#ifdef __GNUC__

extern "C" void __cxa_pure_virtual()
{
}

#endif  // __GNUC__

/**
 * 暫定でディスプレイ出力する。
 */
class ktty_x86 : public ktty
{
	_u32 width;
	_u32 height;
	_u8* vram;
	_u32 cur_row;
	_u32 cur_col;

	void roll(unsigned int n);

public:
	ktty_x86() {}
	void* operator new(std::size_t size);
	void init();
	virtual ktty* putc(char ch);
};

void* ktty_x86::operator new(std::size_t size)
{
	size = size;
	return reinterpret_cast<void*>(0x70000);
}

/**
 * スクロールする。
 *
 * @param n スクロールする行数。
 */
void ktty_x86::roll(unsigned int n)
{
	if (n > height)
		n = height;

	memcpy(&vram[0], &vram[width * 2 * n],
		width * (height - n) * 2);

	_u8* space = &vram[width * (height - n) * 2];
	const int space_size = width * n * 2;
	for (int i = 0; i < space_size; i += 2) {
		space[i] = ' ';
		space[i + 1] = 0x00;
	}
}

void ktty_x86::init()
{
/*
	_u8* param = reinterpret_cast<_u8*>(PH3_4_PARAM_SEG << 4);

	width = *reinterpret_cast<_u32*>(&param[PH3_4_DISP_WIDTH]);
	height = *reinterpret_cast<_u32*>(&param[PH3_4_DISP_HEIGHT]);
	_u32 vram_addr = *reinterpret_cast<_u32*>(&param[PH3_4_DISP_VRAM]);
	vram = reinterpret_cast<_u8*>(vram_addr);

	cur_row = cur_col = 0;
*/
}

/**
 * １文字出力する。
 *
 * @param ch 出力する文字。
 */
ktty* ktty_x86::putc(char ch)
{
	if (ch == '\n') {
		cur_col = 0;
		cur_row++;
		if (cur_row == height) {
			roll(1);
			cur_row--;
		}
	} else {
		const int cur = (width * cur_row + cur_col) * 2;
		vram[cur] = ch;
		vram[cur + 1] = 0x0f;

		if (++cur_col == width) {
			cur_col = 0;
			cur_row++;
			if (cur_row == height) {
				roll(1);
				cur_row--;
			}
		}
	}

	return this;
}

/**
 * コンソールへヌル終端文字列を出力する。
 *
 * @param str ヌル終端文字列。
 */
/*ktty* ktty_x86::puts(const char* str)
{
	while (*str) {
		putc(*str++);
	}

	return this;
}

const char base_number[] = "0123456789abcdefghijklmnopqrstuvwxyz";
*/

/**
 * コンソールへ符号なし３２ビット整数を１０進数で出力する。
 *
 * @param n 符号なし整数値。
 */
/*
ktty* ktty_x86::putu32(_u32 n)
{
	if (n == 0) {
		putc('0');
		return this;
	}

	bool notzero = false;
	for (_u32 div = 1000000000; div > 0; div /= 10) {
		const int x = n / div;
		if (x != 0 || notzero) {
			putc(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return this;
}
*/

/**
 * コンソールへ符号なし３２ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
/*
ktty_x86* ktty_x86::putu32x(_u32 n)
{
	for (int shift = 32 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}
*/

/**
 * コンソールへ符号なし６４ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
/*
ktty_x86* ktty_x86::putu64x(_u64 n)
{
	for (int shift = 64 - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}
*/
ktty* create_ktty()
{
	ktty_x86* p = new ktty_x86;
	p->init();

	return p;
}
