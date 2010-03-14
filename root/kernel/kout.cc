// @file    kernel/kout.cc
// @author  Kato Takeshi
// @brief   Kernel message output destination.
//
// (C) Kato Takeshi 2009-2010

#include "kout.hh"


static const char base_number[] = "0123456789abcdef";

void kern_output::putux(_u64 n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		putc(base_number[x]);
	}
}

kern_output* kern_output::puts(const char* s)
{
	while (*s) {
		putc(*s++);
	}

	return this;
}

kern_output* kern_output::putsd(_s64 n)
{
	if (n < 0) {
		putc('-');
		n = -n;
	}

	return putud(static_cast<_u64>(n));
}

kern_output* kern_output::putud(_u64 n)
{
	if (n == 0) {
		putc('0');
		return this;
	}

	bool notzero = false;
	for (_u64 div = 10000000000000000000ULL; div > 0; div /= 10) {
		const _u64 x = n / div;
		if (x != 0 || notzero) {
			putc(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return this;
}

kern_output* kern_output::putx(_u8 n)
{
	putux(n, 8);

	return this;
}

kern_output* kern_output::putx(_u16 n)
{
	putux(n, 16);

	return this;
}

kern_output* kern_output::putx(_u32 n)
{
	putux(n, 32);

	return this;
}

kern_output* kern_output::putx(_u64 n)
{
	putux(n, 64);

	return this;
}
