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
void console::putc(char ch)
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
}

