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


void kern_output::putux(u64 n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		PutC(base_number[x]);
	}
}

kern_output* kern_output::PutC(char c)
{
	if (!this)
		return this;

	io_vector iov;

	iov.bytes = 1;
	iov.address = &c;

	write(&iov,
		1,  // iov count
		0); // offset

	return this;
}

kern_output* kern_output::PutStr(const char* s)
{
	if (!this)
		return this;

	char buf[32];
	const int buf_size = sizeof buf;
	io_vector iov;

	iov.address = buf;

	if (!s) {
		static const char nullstr[] = "(nullstr)";
		s = nullstr;
	}

	int left = string_get_length(s);
	while (left > 0) {
		iov.bytes = min(left, buf_size);
		memory_move(s, iov.address, iov.bytes);
		write(&iov,
		    1,   // iov count
		    0);  // offset
		s += iov.bytes;
		left -= iov.bytes;
	}

	return this;
}

kern_output* kern_output::PutSDec(s64 n)
{
	if (!this)
		return this;

	if (n < 0) {
		PutC('-');
		n = -n;
	}

	return PutUDec(static_cast<u64>(n));
}

kern_output* kern_output::PutUDec(u64 n)
{
	if (!this)
		return this;

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

kern_output* kern_output::PutU8Hex(u8 n)
{
	if (this)
		putux(n, 8);

	return this;
}

kern_output* kern_output::PutU16Hex(u16 n)
{
	if (this)
		putux(n, 16);

	return this;
}

kern_output* kern_output::PutU32Hex(u32 n)
{
	if (this)
		putux(n, 32);

	return this;
}

kern_output* kern_output::PutU64Hex(u64 n)
{
	if (this)
		putux(n, 64);

	return this;
}
