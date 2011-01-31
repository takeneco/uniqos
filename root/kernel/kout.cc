// @file   kout.cc
// @brief  Kernel debug message output destination.
//
// (C) 2009-2011 KATO Takeshi
//

#include "kout.hh"

#include "string.hh"

namespace {

static const char base_number[] = "0123456789abcdef";

}

void kout::put_ux(u64 n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		write(base_number[x]);
	}
}

kout& kout::c(char ch)
{
	if (!this)
		return *this;

	write(ch);

	return *this;
}

kout& kout::str(const char* s)
{
	if (!this)
		return *this;

	if (!s) {
		static const char nullstr[] = "(nullstr)";
		s = nullstr;
	}

	while (*s) {
		write(*s);
		s++;
	}

	return *this;
}

kout& kout::sdec(s64 n)
{
	if (!this)
		return *this;

	if (n < 0) {
		write('-');
		n = -n;
	}

	return udec(static_cast<u64>(n));
}

kout& kout::udec(u64 n)
{
	if (!this)
		return *this;

	if (n == 0) {
		write('0');
		return *this;
	}

	bool notzero = false;
	for (u64 div = U64CAST(10000000000000000000); div > 0; div /= 10) {
		const u64 x = n / div;
		if (x != 0 || notzero) {
			write(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return *this;
}

kout& kout::u8hex(u8 n)
{
	if (this)
		put_ux(n, 8);

	return *this;
}

kout& kout::u16hex(u16 n)
{
	if (this)
		put_ux(n, 16);

	return *this;
}

kout& kout::u32hex(u32 n)
{
	if (this)
		put_ux(n, 32);

	return *this;
}

kout& kout::u64hex(u64 n)
{
	if (this)
		put_ux(n, 64);

	return *this;
}




void kern_output::putux(u64 n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		put_c(base_number[x]);
	}
}

kern_output* kern_output::put_c(char c)
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

kern_output* kern_output::put_str(const char* s)
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

kern_output* kern_output::put_sdec(s64 n)
{
	if (!this)
		return this;

	if (n < 0) {
		put_c('-');
		n = -n;
	}

	return PutUDec(static_cast<u64>(n));
}

kern_output* kern_output::put_udec(u64 n)
{
	if (!this)
		return this;

	if (n == 0) {
		put_c('0');
		return this;
	}

	bool notzero = false;
	for (u64 div = _u64cast(10000000000000000000); div > 0; div /= 10) {
		const u64 x = n / div;
		if (x != 0 || notzero) {
			put_c(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}

	return this;
}

kern_output* kern_output::put_u8hex(u8 n)
{
	if (this)
		putux(n, 8);

	return this;
}

kern_output* kern_output::put_u16hex(u16 n)
{
	if (this)
		putux(n, 16);

	return this;
}

kern_output* kern_output::put_u32hex(u32 n)
{
	if (this)
		putux(n, 32);

	return this;
}

kern_output* kern_output::put_u64hex(u64 n)
{
	if (this)
		putux(n, 64);

	return this;
}
