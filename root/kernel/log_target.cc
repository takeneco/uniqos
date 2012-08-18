/// @file   log_target.cc
/// @brief  log destination.
//
// (C) 2009-2012 KATO Takeshi
//

#include <log_target.hh>

#include <string.hh>


namespace {

int u_to_str(umax n, int base, char s[sizeof n * 8])
{
	switch (base) {
	case 10: return u_to_decstr(n, s);
	case 16: return u_to_hexstr(n, s);
	case 8:  return u_to_octstr(n, s);
	case 2:  return u_to_binstr(n, s);
	default: s[0] = '?'; return 1;
	}
}

const char NULLSTR[] = "(null)";

enum {
	JUST_LEFT   = 0x01,
	SIGN_PLUS   = 0x02,
	SIGN_SPACE  = 0x04,
	SHAPE       = 0x08,
	PAD_ZERO    = 0x10,
	UPPER       = 0x20,
};

}  // namespace

void file_wr_1vec(
    const void* data,
    uptr bytes,
    file* target)
{
	iovec iov;
	iov.base = const_cast<void*>(data);
	iov.bytes = bytes;

	uptr tmp;
	target->write(&iov, 1, &tmp);
}

void file_wr_rep(char c, int count, file* target)
{
	iovec iov;
	iov.base = &c;
	iov.bytes = 1;

	for (int i = 0; i < count; ++i) {
		uptr tmp;
		target->write(&iov, 1, &tmp);
	}
}

void file_wr_str(
    const char* str,
    file* target)
{
	if (!str)
		str = NULLSTR;

	file_wr_1vec(str, str_length(str), target);
}

void file_wr_fstr(
    const char* str,
    int width,
    int prec,
    bool left,
    file* target)
{
	if (!str)
		str = NULLSTR;

	if (prec == -1) {
		prec = 0x7fffffff;
		//TODO
		//prec = std::numeric_limits<int>::max();
	}

	int len;
	for (len = 0; str[len] && len < prec; ++len)
		; // nothing

	const int space = width - len;

	if (space > 0 && !left)
		file_wr_rep(' ', width - len, target);

	if (len > 0)
		file_wr_1vec(str, len, target);

	if (space > 0 && left)
		file_wr_rep(' ', width - len, target);
}

void file_wr_fhex(
    umax num,
    int width,
    int prec,
    u8 flag,
    file* target)
{
	char hexstr[sizeof (umax) * 2];
	int len = u_to_hexstr(num, hexstr);

	const bool left = (flag & JUST_LEFT) != 0;
	const bool shape = (flag & SHAPE) != 0;
	const bool upper = (flag & UPPER) != 0;
	const bool zero = (flag & PAD_ZERO) != 0;

	if (upper)
		str_to_upper(len, hexstr);

	int pads = width - max(len, prec);
	if (shape)
		pads -= 2;  // length of "0x"

	if (shape && zero)
		file_wr_1vec(upper ? "0X" : "0x", 2, target);

	if (pads > 0 && !left)
		file_wr_rep(zero ? '0' : ' ', pads, target);

	if (shape && !zero)
		file_wr_1vec(upper ? "0X" : "0x", 2, target);

	if (len < prec)
		file_wr_rep('0', prec - len, target);

	file_wr_1vec(hexstr, len, target);

	if (pads > 0 && left)
		file_wr_rep(zero ? '0' : ' ', pads, target);
}

void file_wr_foct(
    umax num,
    int width,
    int prec,
    u8 flag,
    file* target)
{
	char octstr[(sizeof (umax) * 8 + 2) / 3];
	int len = u_to_octstr(num, octstr);

	int pads = width - max(len, prec);
	if (flag & SHAPE)
		pads -= 2;

	if (pads > 0 && !(flag & JUST_LEFT))
		file_wr_rep(flag & PAD_ZERO ? '0' : ' ', pads, target);

	if (flag & SHAPE)
		file_wr_1vec("0", 1, target);

	if (len < prec)
		file_wr_rep('0', prec - len, target);

	file_wr_1vec(octstr, len, target);

	if (pads > 0 && (flag & JUST_LEFT))
		file_wr_rep(flag & PAD_ZERO ? '0' : ' ', pads, target);
}

void log_target::_wr_u(umax n, u8 base, int /*bits*/)
{
	char buf[sizeof n * 8];
	const int bytes = u_to_str(n, base, buf);

	write(buf, bytes);
}

void log_target::_wr_s(smax n, s8 base, int /*bits*/)
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
	iov[1].bytes = u_to_str(n, base, buf);
	iov[1].base = buf;

	uptr wrote;
	target->write(iov, 2, &wrote);
}

void log_target::_wr_p(const void* p)
{
	iovec iov[2];

	const char aster = '*';
	iov[0].base = const_cast<char*>(&aster);
	iov[0].bytes = 1;

	char buf[sizeof p * 2];
	iov[1].bytes =
	    u_to_hexstr(reinterpret_cast<uptr>(p), buf);
	iov[1].base = buf;

	uptr wrote;
	target->write(iov, 2, &wrote);
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
	iov[2].bytes = u_to_str(line, 10, line_buf);
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
	target->write(iov, iov_count, &wrote);
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
		iov[0].bytes = u_to_str(p[i], 16, buf);
		iov[0].base = buf;

		uptr wrote;
		if (i % 16 != 15) {
			target->write(iov, 2, &wrote);
		}
		else {
			target->write(iov, 1, &wrote);
			endl();
		}
	}
}

namespace {

struct field_spec
{
	u8 flag;

	uint width;

	int precision;

	u8 type;
	enum {
		TYPE_DEFAULT,
		TYPE_CHAR,
		TYPE_SHORT,
		TYPE_LONG,
		TYPE_LONGLONG,
	};

	char style;
};

void decode_field(
    const char* field, va_list va, const char** end, field_spec* spec)
{
	spec->flag = 0;
	for (;; ++field) {
		if (*field == '-')
			spec->flag |= JUST_LEFT;
		else if (*field == '+')
			spec->flag |= SIGN_PLUS;
		else if (*field == '0')
			spec->flag |= PAD_ZERO;
		else if (*field == ' ')
			spec->flag |= SIGN_SPACE;
		else if (*field == '#')
			spec->flag |= SHAPE;
		else
			break;
	}

	if (*field == '*') {
		spec->width = va_arg(va, int);
		++field;
	} else {
		spec->width = str_to_u(field, &field, 10);
	}

	if (*field == '.') {
		++field;
		if (*field == '*') {
			spec->precision = va_arg(va, int);
			++field;
		} else {
			spec->precision = str_to_u(field, &field, 10);
		}
	} else {
		spec->precision = -1;
	}

	if (*field == 'h') {
		if (field[1] == 'h') {
			spec->type = field_spec::TYPE_CHAR;
			++field;
		} else {
			spec->type = field_spec::TYPE_SHORT;
		}
		++field;
	} else if (*field == 'l') {
		if (field[1] == 'l') {
			spec->type = field_spec::TYPE_LONGLONG;
			++field;
		} else {
			spec->type = field_spec::TYPE_LONG;
		}
		++field;
	} else {
		spec->type = field_spec::TYPE_DEFAULT;
	}

	switch (*field) {
	case 'd':
	case 'i':
		spec->style = 'd';
		break;

	case 'X':
	case 'E':
	case 'F':
	case 'G':
		spec->flag |= UPPER;
		// FALLTHROUGH

	case 'u':
	case 'o':
	case 'x':
	case 'e':
	case 'f':
	case 'g':
	case 'c':
	case 's':
	case 'p':
	case 'n':
	case '%':
		spec->style = *field;
		break;

	default:
		spec->style = 0;
		break;
	}
	++field;

	*end = field;
}

void fmt_str(va_list va, const field_spec& spec, file* dest)
{
	const char* str = va_arg(va, const char*);

	file_wr_fstr(str,
	             spec.width,
	             spec.precision,
	             spec.flag & JUST_LEFT ? true : false,
	             dest);
}

void fmt_ch(va_list va, const field_spec& spec, log_target* dest)
{
	char c = va_arg(va, int);
	dest->c(c);
}

void fmt_dec(va_list va, const field_spec& spec, log_target* dest)
{
	bool sign = spec.style == 'd';

	u8 type = spec.type;
	if (type == field_spec::TYPE_DEFAULT)
		type = field_spec::TYPE_LONG;

	if (type == field_spec::TYPE_CHAR) {
		s8 val = va_arg(va, s8);
		sign ? log_wr_s(dest, val, 10, 8) :
		       log_wr_u(dest, val, 10, 8);
	} else if (type == field_spec::TYPE_SHORT) {
		s16 val = va_arg(va, s16);
		sign ? log_wr_s(dest, val, 10, 16) :
		       log_wr_u(dest, val, 10, 16);
	} else if (type == field_spec::TYPE_LONG) {
		s32 val = va_arg(va, s32);
		sign ? log_wr_s(dest, val, 10, 32) :
		       log_wr_u(dest, val, 10, 32);
	} else if (type == field_spec::TYPE_LONGLONG) {
		s64 val = va_arg(va, s64);
		sign ? log_wr_s(dest, val, 10, 64) :
		       log_wr_u(dest, val, 10, 64);
	}
}

void fmt_oct(va_list va, const field_spec& spec, file* dest)
{
	u8 type = spec.type;
	if (type == field_spec::TYPE_DEFAULT)
		type = field_spec::TYPE_LONG;

	if (type == field_spec::TYPE_CHAR) {
		u8 val = va_arg(va, u8);
		file_wr_fhex(val, spec.width, spec.precision, spec.flag, dest);
	}
}

void fmt_hex(va_list va, const field_spec& spec, file* dest)
{
	u8 type = spec.type;
	if (type == field_spec::TYPE_DEFAULT)
		type = field_spec::TYPE_LONG;

	if (type == field_spec::TYPE_CHAR) {
		u8 val = va_arg(va, u8);
		//log_wr_u(dest, val, 16, 8);
		file_wr_fhex(val, spec.width, spec.precision, spec.flag, dest);
	} else if (type == field_spec::TYPE_SHORT) {
		u16 val = va_arg(va, u16);
		//log_wr_u(dest, val, 16, 16);
		file_wr_fhex(val, spec.width, spec.precision, spec.flag, dest);
	} else if (type == field_spec::TYPE_LONG) {
		u32 val = va_arg(va, u32);
		//log_wr_u(dest, val, 16, 32);
		file_wr_fhex(val, spec.width, spec.precision, spec.flag, dest);
	} else if (type == field_spec::TYPE_LONGLONG) {
		u64 val = va_arg(va, u64);
		//log_wr_u(dest, val, 16, 64);
		file_wr_fhex(val, spec.width, spec.precision, spec.flag, dest);
	}
}

}  // namespace

void log_target::_wr_fmt(const char* fmt, va_list va)
{
	const char* raw_out_pos = fmt;
	uptr raw_out_len = 0;

	while (*fmt) {
		if (*fmt == '%') {
			if (raw_out_len != 0) {
				write(raw_out_pos, raw_out_len);
				raw_out_len = 0;
			}

			++fmt;

			const char* end;
			field_spec spec;
			decode_field(fmt, va, &end, &spec);

			switch (spec.style) {
			case 's':
				fmt_str(va, spec, target);
				break;

			case 'c':
				fmt_ch(va, spec, this);
				break;

			case 'd':
			case 'u':
				fmt_dec(va, spec, this);
				break;

			case 'o':
				fmt_oct(va, spec, target);
				break;

			case 'p':
				spec.style = 'x';
				fmt_hex(va, spec, target);
				break;

			case 'X':
			case 'x':
				fmt_hex(va, spec, target);
				break;

			case '%':
				write("%", 1);
				break;

			default:
				write(fmt-1, end - fmt+1);

				file_wr_str("{", target);
				_wr_u(spec.flag, 16, 8);
				file_wr_str(",", target);
				_wr_u(spec.width, 10, 32);
				file_wr_str(",", target);
				_wr_u(spec.precision, 10, 32);
				file_wr_str(",", target);
				_wr_u(spec.type, 10, 8);
				write(&spec.style, 1);
				file_wr_str("}", target);

				va_arg(va, uptr);
				break;
			}
			fmt = end;

			raw_out_pos = fmt;

		} else if (*fmt == '\n') {
			if (raw_out_len != 0) {
				write(raw_out_pos, raw_out_len);
				raw_out_len = 0;
			}

			++fmt;

			endl();

			raw_out_pos = fmt;
		} else {
			++raw_out_len;
			++fmt;
		}
	}

	if (raw_out_len != 0)
		write(raw_out_pos, raw_out_len);
}

void log_wr_str(log_target* x, const char* s)
{
	if (x)
		file_wr_str(s, x->target);
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

void log_wr_fmt(log_target* x, const char* fmt, va_list va)
{
	if (x)
		x->_wr_fmt(fmt, va);
}

