/* FILE : arch/x86/boot/phase4/console.c
 * VER  : 0.0.1
 * LAST : 2009-05-30
 * (C) Kato.T 2009
 *
 * 簡単なテキストコンソール画面出力。
 */

#include "phase4.hpp"

/**
 * console クラスを初期化する。
 *
 * @param w コンソールの横幅（半角文字数）。
 * @param h コンソールの縦幅。
 * @vram_addr VRAM の先頭アドレス。
 */
void console::init(int w, int h, _u32 vram_addr)
{
	width = w;
	height = h;
	vram = reinterpret_cast<char*>(vram_addr);

	curx = cury = 0;
}

/**
 * コンソールへ１文字出力する。
 *
 * @param ch 出力する文字。
 */
console* console::putc(char ch)
{
	if (ch == '\n') {
		curx = 0;
		cury++;
	} else {
		const int cur = (width * cury + curx) * 2;
		vram[cur] = ch;
		vram[cur + 1] = 0x0f;

		if (++curx == width) {
			curx = 0;
			cury++;
		}
	}

	return this;
}

/**
 * コンソールへヌル終端文字列を出力する。
 *
 * @param str ヌル終端文字列。
 */
console* console::puts(const char* str)
{
	while (*str) {
		putc(*str++);
	}

	return this;
}

const char base_number[] = "0123456789abcdefghijklmnopqrstuvwxyz";
/** コンソールへ符号なし３２ビット整数を１０進数で出力する。
 *
 * @param n 符号なし整数値。
 */
console* console::putu32(_u32 n)
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

/** コンソールへ符号なし３２ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
console* console::putu32x(_u32 n)
{
	for (_u32 shift = 32 - 4; ; shift -= 4) {
		const int x = (n >> shift) & 0x0000000f;
		putc(base_number[x]);
		if (shift == 0)
			break;
	}

	return this;
}

