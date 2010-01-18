/**
 * @file    kernel/ktty.cpp
 * @version 0.0.1.0
 * @date    2009-08-02
 * @author  Kato.T
 * @brief   カーネルのメッセージ出力。
 */
// (C) Kato.T 2009

#include "ktty.hpp"


static const char base_number[] =
	"0123456789abcdefghijklmnopqrstuvwxyz";

void ktty::putux(_u64 n, _ucpu bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}
}

ktty* ktty::puts(const char* s)
{
	while (*s) {
		putc(*s++);
	}

	return this;
}

ktty* ktty::putsd(int n)
{
	int digit;

	for (digit = 1000000000; digit > 0; digit /= 10) {
		int a = n / digit;
		if (a != 0) {
			continue;
		}
	}

	for (; digit > 0; digit /= 10) {
		int a = n / digit;
		putc(base_number[a]);
		n %= digit;
	}

	return this;
}

ktty* ktty::put8x(_u8 n)
{
	putux(n, 8);

	return this;
}

ktty* ktty::put16x(_u16 n)
{
	putux(n, 16);

	return this;
}

ktty* ktty::put32x(_u32 n)
{
	putux(n, 32);

	return this;
}

ktty* ktty::put64x(_u64 n)
{
	putux(n, 64);

	return this;
}
