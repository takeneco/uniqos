/**
 * @file    arch/x86_64/kernel/setup/termchain.cpp
 * @author  Kato Takeshi
 * @brief   複数の outterm へ同時にテキスト出力する。
 *
 * (C) Kato.T 2009-2010
 */

#include "term.hh"


/**
 * @brief  Initialize term_chain.
 */
void term_chain::init()
{
	terms = 0;
}

/**
 * @brief  Add outterm to chain.
 */
void term_chain::add_term(outterm* term)
{
	term->next = terms;
	terms = term;
}

/**
 * @brief   Output 1 character.
 *
 * @param c  Charcter.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putc(char c)
{
	for (outterm* p = terms; p; p = p->next) {
		p->putc(c);
	}

	return this;
}

/**
 * @brief   Output null terminal string.
 *
 * @param str  String.
 * @return  Always return this ptr.
 */
term_chain* term_chain::puts(const char* str)
{
	while (*str) {
		putc(*str++);
	}

	return this;
}

const char base_number[] = "0123456789abcdef";

/**
 * @brief   Output _u64 value with decimal.
 *
 * @param n  Value.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putu64(_u64 n)
{
	if (n == 0) {
		putc('0');
		return this;
	}

	bool notzero = false;
	for (_u64 div = 10000000000000000000UL; div > 0; div /= 10) {
		const _u64 x = n / div;
		if (x != 0 || notzero) {
			putc(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return this;
}

/**
 * @brief   Output _u8 value with hexadecimal.
 *
 * @param n  Value.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putu8x(_u8 n)
{
	for (int shift = 8 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * @brief   Output _u16 value with hexadecimal.
 *
 * @param n  Value.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putu16x(_u16 n)
{
	for (int shift = 16 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * @brief   Output _u32 value with hexadecimal.
 *
 * @param n  Value.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putu32x(_u32 n)
{
	for (int shift = 32 - 4; shift >= 0; shift -= 4) {
		const int x = (n >> shift) & 0x0f;
		putc(base_number[x]);
	}

	return this;
}

/**
 * @brief   Output _u64 value with hexadecimal.
 *
 * @param n  Value.
 * @return  Always return this ptr.
 */
term_chain* term_chain::putu64x(_u64 n)
{
	for (int shift = 64 - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0f;
		putc(base_number[x]);
	}

	return this;
}
