/**
 * @file    arch/x86/boot/phase4/termchain.cpp
 * @version 0.0.1
 * @date    2009-07-20
 * @author  Kato.T
 *
 * 複数の outterm へ同時にテキスト出力する。
 */
// (C) Kato.T 2009

#include <cstddef>
#include "phase4.hpp"


/**
 * term_chain を初期化する。
 */
void term_chain::init()
{
	terms = NULL;
}

/**
 * チェインに outterm を追加する。
 */
void term_chain::add_term(outterm* term)
{
	term->next = terms;
	terms = term;
}

/**
 * １文字出力する。
 *
 * @param ch 出力する文字。
 * @return this を返す。
 */
term_chain* term_chain::putc(char ch)
{
	for (outterm* p = terms; p; p = p->next) {
		p->putc(ch);
	}

	return this;
}

/**
 * ヌル終端文字列を出力する。
 *
 * @param str ヌル終端文字列。
 */
term_chain* term_chain::puts(const char* str)
{
	while (*str) {
		putc(*str++);
	}

	return this;
}

const char base_number[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/**
 * 符号なし３２ビット整数を１０進数で出力する。
 *
 * @param n 符号なし整数値。
 */
term_chain* term_chain::putu32(_u32 n)
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

/**
 * 符号なし８ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
term_chain* term_chain::putu8x(_u8 n)
{
	for (int shift = 8 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * 符号なし１６ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
term_chain* term_chain::putu16x(_u16 n)
{
	for (int shift = 16 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * 符号なし３２ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
term_chain* term_chain::putu32x(_u32 n)
{
	for (int shift = 32 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * 符号なし６４ビット整数を１６進数で出力する。
 *
 * @param n 符号なし整数値。
 */
term_chain* term_chain::putu64x(_u64 n)
{
	for (int shift = 64 - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}

	return this;
}
