/// @file   kout.cc
/// @brief  Kernel debug message output destination.
//
// (C) 2009-2011 KATO Takeshi
//

#include "kout.hh"

#include "string.hh"


namespace {

static const char base_number[] = "0123456789abcdef";

}

void kout::put_uhex(umax n, int bits)
{
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		write(base_number[x]);
	}
}

void kout::put_uoct(umax n, int bits)
{
	for (int shift = bits - 3; shift >= 0; shift -= 3) {
		const int x = static_cast<int>(n >> shift) & 0x00000007;
		write(base_number[x]);
	}
}

void kout::put_ubin(umax n, int bits)
{
	for (int shift = bits - 1; shift >= 0; shift -= 1) {
		const int x = static_cast<int>(n >> shift) & 0x00000001;
		write(base_number[x]);
	}
}

void kout::put_udec(u64 n)
{
	if (n == 0)
		write('0');

	bool notzero = false;
	for (u64 div = U64CAST(10000000000000000000); div > 0; div /= 10) {
		const u64 x = n / div;
		if (x != 0 || notzero) {
			write(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}
}

kout& kout::c(char ch)
{
	if (this)
		write(ch);

	return *this;
}

kout& kout::str(const char* s)
{
	if (this) {
		if (!s) {
			static const char nullstr[] = "(nullstr)";
			s = nullstr;
		}

		while (*s) {
			write(*s);
			s++;
		}
	}

	return *this;
}

kout& kout::s(s8 n, int base)
{
	if (this) {
		if (n < 0) {
			write('-');
			u(static_cast<u8>(-n), base);
		} else {
			write('+');
			u(static_cast<u8>(n), base);
		}
	}

	return *this;
}

kout& kout::s(s16 n, int base)
{
	if (this) {
		if (n < 0) {
			write('-');
			u(static_cast<u16>(-n), base);
		} else {
			write('+');
			u(static_cast<u8>(n), base);
		}
	}

	return *this;
}

kout& kout::s(s32 n, int base)
{
	if (this) {
		if (n < 0) {
			write('-');
			u(static_cast<u32>(-n), base);
		} else {
			write('+');
			u(static_cast<u32>(n), base);
		}
	}

	return *this;
}

kout& kout::s(s64 n, int base)
{
	if (this) {
		if (n < 0) {
			write('-');
			u(static_cast<u64>(-n), base);
		} else {
			write('+');
			u(static_cast<u32>(n), base);
		}
	}

	return *this;
}

kout& kout::u(u8 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 8); break;
		case 8:  put_uoct(n, 8); break;
		case 2:  put_ubin(n, 8); break;
		default: write('?'); break;
		}

	return *this;
}

kout& kout::u(u16 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 16); break;
		case 8:  put_uoct(n, 16); break;
		case 2:  put_ubin(n, 16); break;
		default: write('?'); break;
		}

	return *this;
}

kout& kout::u(u32 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 32); break;
		case 8:  put_uoct(n, 32); break;
		case 2:  put_ubin(n, 32); break;
		default: write('?'); break;
		}

	return *this;
}

kout& kout::u(u64 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 64); break;
		case 8:  put_uoct(n, 64); break;
		case 2:  put_ubin(n, 64); break;
		default: write('?'); break;
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
	if (this)
		put_udec(n);

	return *this;
}

kout& kout::u8hex(u8 n)
{
	if (this)
		put_uhex(n, 8);

	return *this;
}

kout& kout::u16hex(u16 n)
{
	if (this)
		put_uhex(n, 16);

	return *this;
}

kout& kout::u32hex(u32 n)
{
	if (this)
		put_uhex(n, 32);

	return *this;
}

kout& kout::u64hex(u64 n)
{
	if (this)
		put_uhex(n, 64);

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
