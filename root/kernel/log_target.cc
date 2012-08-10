/// @file   log_target.cc
/// @brief  log destination.
//
// (C) 2009-2012 KATO Takeshi
//

#include <log_target.hh>

#include <string.hh>


namespace {

static const char base_number[] = "0123456789abcdef";

int u_to_hexstr(umax n, int bits, char s[sizeof (umax) * 2])
{
	int m = 0;
	for (int shift = bits - 4; shift >= 0; shift -= 4) {
		const int x = static_cast<char>(n >> shift) & 0x0f;
		s[m++] = base_number[x];
	}

	return m;
}

int u_to_octstr(umax n, int bits, char s[(sizeof (umax) * 8 + 2) / 3])
{
	int m = 0;
	for (int shift = bits - 3; shift >= 0; shift -= 3) {
		const int x = static_cast<int>(n >> shift) & 0x07;
		s[m++] = base_number[x];
	}

	return m;
}

int u_to_binstr(umax n, int bits, char s[sizeof n * 8])
{
	int m = 0;
	for (int shift = bits - 1; shift >= 0; shift -= 1) {
		const int x = static_cast<int>(n >> shift) & 0x01;
		s[m++] = base_number[x];
	}

	return m;
}

int u_to_decstr(umax n, char s[sizeof n * 3])
{
	enum { maxdigs = sizeof n * 3 };
	int i;
	for (i = 1; i <= maxdigs; ++i) {
		s[maxdigs - i] = base_number[n % 10];
		n = n / 10;
		if (n == 0)
			break;
	}

	mem_move(i, &s[maxdigs - i], s);

	return i;
}

int u_to_str(umax n, int base, int bits, char s[sizeof n * 8])
{
	switch (base) {
	case 10: return u_to_decstr(n, s);
	case 16: return u_to_hexstr(n, bits, s);
	case 8:  return u_to_octstr(n, bits, s);
	case 2:  return u_to_binstr(n, bits, s);
	default: s[0] = '?'; return 1;
	}
}

}  // namespace

void log_target::_wr_str(const char* s)
{
	if (!s) {
		static const char nullstr[] = "(nullstr)";
		s = nullstr;
	}

	write(s, str_length(s));
}

void log_target::_wr_u(umax n, u8 base, int bits)
{
	char buf[sizeof n * 8];
	const int bytes = u_to_str(n, base, bits, buf);

	write(buf, bytes);
}

void log_target::_wr_s(smax n, s8 base, int bits)
{
	const char minus = '-';
	const char plus = '+';

	iovec iov[2];
	if (n < 0) {
		iov[0].base = const_cast<char*>(&minus);
		iov[0].bytes = 1;
		n = -n;
	} else {
		iov[0].base = const_cast<char*>(&plus);
		iov[0].bytes = 1;
	}

	char buf[sizeof n * 8];
	iov[1].bytes = u_to_str(n, base, bits, buf);
	iov[1].base = buf;

	uptr wrote;
	target->call_write(iov, 2, &wrote);
}

void log_target::_wr_p(const void* p)
{
	iovec iov[2];

	const char aster = '*';
	iov[0].base = const_cast<char*>(&aster);
	iov[0].bytes = 1;

	char buf[sizeof p * 2];
	iov[1].bytes =
	    u_to_hexstr(reinterpret_cast<uptr>(p), sizeof p * 8, buf);
	iov[1].base = buf;

	uptr wrote;
	target->call_write(iov, 2, &wrote);
}

/// @param func  function name. null available.
void log_target::_wr_src(const char* path, int line, const char* func)
{
	iovec iov[5];

	iov[0].base = const_cast<char*>(path);
	iov[0].bytes = str_length(path);

	const char sep1 = ':';
	iov[1].base = const_cast<char*>(&sep1);
	iov[1].bytes = 1;

	char line_buf[10];
	iov[2].bytes = u_to_str(line, 10, 0, line_buf);
	iov[2].base = line_buf;

	const char sep2 = ':';
	iov[3].base = const_cast<char*>(&sep2);
	iov[3].bytes = 1;

	int iov_count;
	if (func) {
		iov[4].base = const_cast<char*>(func);
		iov[4].bytes = str_length(func);

		iov_count = 5;
	} else {
		iov_count = 4;
	}

	uptr wrote;
	target->call_write(iov, iov_count, &wrote);
}

void log_target::_wr_bin(uptr bytes, const void* data)
{
	const char sep = ' ';
	iovec iov[2];
	iov[1].bytes = 1;
	iov[1].base = const_cast<char*>(&sep);

	const u8* p = static_cast<const u8*>(data);

	for (uptr i = 0; i < bytes; ++i) {
		char buf[2];
		iov[0].bytes = u_to_str(p[i], 16, 8, buf);
		iov[0].base = buf;

		uptr wrote;
		if (i % 16 != 15) {
			target->call_write(iov, 2, &wrote);
		}
		else {
			target->call_write(iov, 1, &wrote);
			endl();
		}
	}
}

void log_wr_str(log_target* x, const char* s)
{
	if (x)
		x->_wr_str(s);
}

void log_wr_u(log_target* x, umax n, int base, int bits)
{
	if (x)
		x->_wr_u(n, base, bits);
}

void log_wr_s(log_target* x, smax n, int base, int bits)
{
	if (x)
		x->_wr_s(n, base, bits);
}

void log_wr_p(log_target* x, const void* p)
{
	if (x)
		x->_wr_p(p);
}

void log_wr_src(log_target* x, const char* path, int line, const char* func)
{
	if (x)
		x->_wr_src(path, line, func);
}

void log_wr_bin(log_target* x, uptr bytes, const void* data)
{
	if (x)
		x->_wr_bin(bytes, data);
}

