// @file    kernel/kout.cc
// @author  Kato Takeshi
// @brief   Kernel message output destination.
//
// (C) 2009-2010 Kato Takeshi.

#include "kout.hh"

#include "string.hh"

namespace {

static const char base_number[] = "0123456789abcdef";

}


void KernOutput::putux(u64 n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		PutC(base_number[x]);
	}
}

KernOutput* KernOutput::PutC(char c)
{
	IOVector iov;

	iov.Bytes = 1;
	iov.Address = &c;

	Write(&iov,
		1,  // iov count
		0); // offset

	return this;
}

KernOutput* KernOutput::PutStr(const char* s)
{
	IOVector iov;

	iov.Bytes = string_get_length(s);
	iov.Address = s;

	Write(&iov,
		1,  // iov count
		0); // offset

	return this;
}

KernOutput* KernOutput::PutSDec(s64 n)
{
	if (n < 0) {
		PutC('-');
		n = -n;
	}

	return PutUDec(static_cast<u64>(n));
}

KernOutput* KernOutput::PutUDec(u64 n)
{
	if (n == 0) {
		PutC('0');
		return this;
	}

	bool notzero = false;
	for (u64 div = _u64cast(10000000000000000000); div > 0; div /= 10) {
		const u64 x = n / div;
		if (x != 0 || notzero) {
			PutC(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return this;
}

KernOutput* KernOutput::PutU8Hex(u8 n)
{
	putux(n, 8);

	return this;
}

KernOutput* KernOutput::PutU16Hex(u16 n)
{
	putux(n, 16);

	return this;
}

KernOutput* KernOutput::PutU32Hex(u32 n)
{
	putux(n, 32);

	return this;
}

KernOutput* KernOutput::PutU64Hex(u64 n)
{
	putux(n, 64);

	return this;
}
