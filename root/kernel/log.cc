/// @file   log.cc
/// @brief  Kernel log output destination.
//
// (C) 2009-2011 KATO Takeshi
//

#include "log.hh"

#include "string.hh"


// log_target

namespace {

static const char base_number[] = "0123456789abcdef";

}

void log_target::put_uhex(umax n, int bits)
{
	char s[sizeof n * 2];
	u32 m = 0;
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<int>(n >> shift) & 0x0000000f;
		s[m++] = base_number[x];
	}
	call_write(s, m);
}

void log_target::put_uoct(umax n, int bits)
{
	char s[(sizeof n * 8 + 2) / 3];
	u32 m = 0;
	for (int shift = bits - 3; shift >= 0; shift -= 3) {
		const int x = static_cast<int>(n >> shift) & 0x00000007;
		s[m++] = base_number[x];
	}
	call_write(s, m);
}

void log_target::put_ubin(umax n, int bits)
{
	char s[sizeof n * 8]; // BITS_PER_BYTE
	u32 m = 0;
	for (int shift = bits - 1; shift >= 0; shift -= 1) {
		const int x = static_cast<int>(n >> shift) & 0x00000001;
		s[m++] = base_number[x];
	}
	call_write(s, m);
}

void log_target::put_udec(uptr n)
{
	enum { maxdigs = 20 };
	char s[maxdigs];
	int i;
	for (i = 1; i <= maxdigs; ++i) {
		s[maxdigs - i] = base_number[n % 10];
		n = n / 10;
		if (n == 0)
			break;
	}
	call_write(&s[maxdigs - i], i);
}

log_target& log_target::c(char ch)
{
	if (this)
		call_write(&ch, 1);

	return *this;
}

log_target& log_target::str(const char* s)
{
	if (this) {
		if (!s) {
			static const char nullstr[] = "(nullstr)";
			s = nullstr;
		}

		call_write(s, string_get_length(s));
	}

	return *this;
}

namespace {

inline void write_minus(log_target* self)
{
	const char minus = '-';
	self->call_write(&minus, 1);
}

inline void write_plus(log_target* self)
{
	const char plus = '+';
	self->call_write(&plus, 1);
}

inline void write_unknown(log_target* self)
{
	const char unknown = '?';
	self->call_write(&unknown, 1);
}

} // namespace

log_target& log_target::s(s8 n, int base)
{
	if (this) {
		if (n < 0) {
			write_minus(this);
			u(static_cast<u8>(-n), base);
		} else {
			write_plus(this);
			u(static_cast<u8>(n), base);
		}
	}

	return *this;
}

log_target& log_target::s(s16 n, int base)
{
	if (this) {
		if (n < 0) {
			write_minus(this);
			u(static_cast<u16>(-n), base);
		} else {
			write_plus(this);
			u(static_cast<u16>(n), base);
		}
	}

	return *this;
}

log_target& log_target::s(s32 n, int base)
{
	if (this) {
		if (n < 0) {
			write_minus(this);
			u(static_cast<u32>(-n), base);
		} else {
			write_plus(this);
			u(static_cast<u32>(n), base);
		}
	}

	return *this;
}

log_target& log_target::s(s64 n, int base)
{
	if (this) {
		if (n < 0) {
			write_minus(this);
			u(static_cast<u64>(-n), base);
		} else {
			write_plus(this);
			u(static_cast<u64>(n), base);
		}
	}

	return *this;
}

log_target& log_target::u(u8 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 8); break;
		case 8:  put_uoct(n, 8); break;
		case 2:  put_ubin(n, 8); break;
		default: write_unknown(this); break;
		}

	return *this;
}

log_target& log_target::u(u16 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 16); break;
		case 8:  put_uoct(n, 16); break;
		case 2:  put_ubin(n, 16); break;
		default: write_unknown(this); break;
		}

	return *this;
}

log_target& log_target::u(u32 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 32); break;
		case 8:  put_uoct(n, 32); break;
		case 2:  put_ubin(n, 32); break;
		default: write_unknown(this); break;
		}

	return *this;
}

log_target& log_target::u(u64 n, int base)
{
	if (this)
		switch (base) {
		case 10: put_udec(n); break;
		case 16: put_uhex(n, 64); break;
		case 8:  put_uoct(n, 64); break;
		case 2:  put_ubin(n, 64); break;
		default: write_unknown(this); break;
		}

	return *this;
}

log_target& log_target::sdec(s64 n)
{
	if (!this)
		return *this;

	if (n < 0) {
		writec_func(this, '-');
		n = -n;
	}

	return udec(static_cast<u64>(n));
}

log_target& log_target::udec(u64 n)
{
	if (this)
		put_udec(n);

	return *this;
}

log_target& log_target::u8hex(u8 n)
{
	if (this)
		put_uhex(n, 8);

	return *this;
}

log_target& log_target::u16hex(u16 n)
{
	if (this)
		put_uhex(n, 16);

	return *this;
}

log_target& log_target::u32hex(u32 n)
{
	if (this)
		put_uhex(n, 32);

	return *this;
}

log_target& log_target::u64hex(u64 n)
{
	if (this)
		put_uhex(n, 64);

	return *this;
}

cause::stype log_target::default_write_func(
    log_target* self, const void* data, u32 bytes)
{
	for (u32 i = 0; i < bytes; ++i)
		self->writec_func(self, reinterpret_cast<const u8*>(data)[i]);
	return cause::OK;
}

// log_io

cause::stype log_file::write(log_target* self, const void* data, u32 bytes)
{
	log_file* this_ = reinterpret_cast<log_file*>(self);

	return this_->target->ops->write(this_->target, data, bytes, 0);
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
		mem_move(iov.bytes, s, iov.address);
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
/*
	bool notzero = false;
	for (u64 div = U64(10000000000000000000); div > 0; div /= 10) {
		const u64 x = n / div;
		if (x != 0 || notzero) {
			put_c(base_number[x]);
			n -= x * div;
			notzero = true;
		}
	}
*/
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
