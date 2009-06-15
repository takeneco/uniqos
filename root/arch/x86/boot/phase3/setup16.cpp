/* FILE : arch/x86/boot/phase3/setup16.cpp
 * VER  : 0.0.1
 * LAST : 2009-05-25
 * (C) Kato.T 2009
 *
 * phase3 から呼び出す処理。
 */

#include "types.hpp"
#include "boot.h"

asm (".code16gcc");

extern "C" void setup16(void);

/**
 * BIOSで1文字を表示する。
 * @param ch 表示する文字。
 * @param col 表示色。
 */
static inline void bios_putch(char ch, _u8 col=15)
{
	const _u16 b = col;
	asm volatile ("int $0x10" : : "a" (0x0e00 | ch), "b" (b));
}

/**
 * BIOSで文字列を表示する。
 * @param str 表示するヌル終端文字列。
 * @param col 表示色。
 */
static void bios_putstr(const char* str, _u8 col=15)
{
	while (*str) {
		bios_putch(*str++, col);
	}
}

static const char chbase[] = "0123456789abcdef";

/**
 * ８ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
static void bios_put8_b16(_u8 x, _u8 col=15)
{
	bios_putch(chbase[x >> 4], col);
	bios_putch(chbase[x & 0xf], col);
}

/**
 * １６ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
static void bios_put16_b16(_u16 x, _u8 col=15)
{
	bios_putch(chbase[(x >>12) & 0xf], col);
	bios_putch(chbase[(x >> 8) & 0xf], col);
	bios_putch(chbase[(x >> 4) & 0xf], col);
	bios_putch(chbase[x & 0xf], col);
}

/**
 * キーボード入力を待ってリブートする。
 */
void keyboard_and_reboot()
{
	asm volatile("int $0x16; int $0x19" : : "a"(0));
}

/**
 * 32ビットプロテクトモード移行の準備をする。
 */
void setup16()
{
}

