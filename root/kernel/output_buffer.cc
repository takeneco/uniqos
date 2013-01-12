/// @file   output_buffer.cc
/// @brief  Text output utilities.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <output_buffer.hh>

#include <arch.hh>
#include <bitops.hh>
#include <string.hh>


namespace {

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


void output_buffer_vec(output_buffer* x, int iov_cnt, const iovec* iov)
{
	x->_vec(iov_cnt, iov);
}

void output_buffer_1vec(output_buffer* x, uptr bytes, const void* data)
{
	x->_1vec(bytes, data);
}

/// @brief Output null-terminated string.
/// @param     x     output_buffer instance.
/// @param[in] str   出力する文字列。
/// @param[in] width 最小限出力する幅。str が width より短い場合は
///                  str の後を ' ' で埋める。
void output_buffer_str(output_buffer* x, const char* str, int width)
{
	if (!str)
		str = NULLSTR;

	const int len = str_length(str);

	x->_1vec(len, str);

	x->_rep(width - len, ' ');
}

void output_buffer_strf(
    output_buffer* x,
    const char* str,
    int width,
    int prec,
    u8 flags)
{
	const bool left = (flags & JUST_LEFT) != 0;

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
		x->_rep(width - len, ' ');

	if (len > 0)
		x->_1vec(len, str);

	if (space > 0 && left)
		x->_rep(width - len, ' ');
}

/// @brief 符号なし１０進出力
void output_buffer_u(output_buffer* x, umax num, int width)
{
	char buf[sizeof num * 3];
	const int len = u_to_decstr(num, buf);

	x->_rep(width - len, ' ');
	x->_1vec(len, buf);
}

void output_buffer_uf(output_buffer* x, umax num, int width, int prec, u8 flags)
{
	char decstr[sizeof num * 3];
	int len = u_to_decstr(num, decstr);

	const bool left = (flags & JUST_LEFT) != 0;
	const bool plus = (flags & SIGN_PLUS) != 0;
	const bool space = (flags & SIGN_SPACE) != 0;
	const bool zero = (flags & PAD_ZERO) != 0;

	int pads = width - max(len, prec);
	if (plus || space)
		pads -= 1;  // length of '+' or ' '

	if (pads > 0 && !left && !zero)
		x->_rep(pads, ' ');

	if (plus || space)
		x->_1vec(1, plus ? "+" : " ");

	if (pads > 0 && !left && zero)
		x->_rep(pads, '0');

	if (len < prec)
		x->_rep(prec - len, '0');

	x->_1vec(len, decstr);

	if (pads > 0 && left)
		x->_rep(pads, ' ');
}

/// @brief 強制符号付１０進出力
void output_buffer_s(output_buffer* x, smax num, int width)
{
	char sign;
	if (num < 0) {
		sign = '-';
		num = -num;
	} else {
		sign = '+';
	}

	iovec iov[2];
	iov[0].base = &sign;
	iov[0].bytes = 1;

	char buf[sizeof num * 3];
	iov[1].bytes = u_to_decstr(num, buf);
	iov[1].base = buf;

	x->_rep(width - (iov[1].bytes + 1), ' ');
	x->_vec(2, iov);
}

void output_buffer_sf(output_buffer* x, smax num, int width, int prec, u8 flags)
{
	char decstr[sizeof num * 3];
	int len = u_to_decstr(num, decstr);

	const bool left = (flags & JUST_LEFT) != 0;
	const bool plus = (flags & SIGN_PLUS) != 0;
	const bool space = (flags & SIGN_SPACE) != 0;
	const bool zero = (flags & PAD_ZERO) != 0;

	int pads = width - max(len, prec);
	if (plus || space || num < 0)
		pads -= 1;  // length of '+', '-' or ' '

	if (!left && !zero && pads > 0)
		x->_rep(pads, ' ');

	char sign_ch = 0;
	if (num < 0)    sign_ch = '-';
	else if (plus)  sign_ch = '+';
	else if (space) sign_ch = ' ';
	if (sign_ch)
		x->_1vec(1, &sign_ch);

	if (!left && zero && pads > 0)
		x->_rep(pads, '0');

	if (left && prec > len)
		x->_rep(prec - len, '0');

	x->_1vec(len, decstr);

	if (left && pads > 0)
		x->_rep(pads, ' ');
}

/// @brief 符号なし１６進出力
void output_buffer_hex(output_buffer* x, umax num, int width)
{
	char buf[sizeof num * 2];
	const int len = u_to_hexstr(num, buf);

	x->_rep(width - len, ' ');
	x->_1vec(len, buf);
}

void output_buffer_hexf(
    output_buffer* x, umax num, int width, int prec, u8 flags)
{
	char hexstr[sizeof (umax) * 2];
	int len = u_to_hexstr(num, hexstr);

	const bool left = (flags & JUST_LEFT) != 0;
	const bool shape = (flags & SHAPE) != 0;
	const bool upper = (flags & UPPER) != 0;
	const bool zero = (flags & PAD_ZERO) != 0;

	if (upper)
		str_to_upper(len, hexstr);

	int pads = width - max(len, prec);
	if (shape)
		pads -= 2;  // length of "0x"

	if (pads > 0 && !left && !zero)
		x->_rep(pads, ' ');

	if (shape)
		x->_1vec(2, upper ? "0X" : "0x");

	if (pads > 0 && !left && zero)
		x->_rep(pads, '0');

	if (len < prec)
		x->_rep(prec - len, '0');

	x->_1vec(len, hexstr);

	if (pads > 0 && left)
		x->_rep(pads, ' ');
}

/// @brief 符号なし８進出力
void output_buffer_oct(output_buffer* x, umax num, int width)
{
	char buf[(sizeof (umax) * 8 + 2) / 3];
	const int len = u_to_octstr(num, buf);

	x->_rep(width - len, '0');
	x->_1vec(len, buf);
}

void output_buffer_octf(
    output_buffer* x, umax num, int width, int prec, u8 flags)
{
	char octstr[(sizeof (umax) * 8 + 2) / 3];
	int len = u_to_octstr(num, octstr);

	const bool left = (flags & JUST_LEFT) != 0;
	const bool shape = (flags & SHAPE) != 0;
	const bool zero = (flags & PAD_ZERO) != 0;

	int pads = width - max(len, prec);
	if (shape)
		pads -= 1;  // length of "0"

	if (pads > 0 && !left && !zero)
		x->_rep(pads, ' ');

	if (shape)
		x->_1vec(1, "0");

	if (pads > 0 && !left && zero)
		x->_rep(pads, '0');

	if (len < prec)
		x->_rep(prec - len, '0');

	x->_1vec(len, octstr);

	if (pads > 0 && left)
		x->_rep(pads, ' ');
}

void output_buffer_adr(output_buffer* x, const void* ptr)
{
	char buf[1 + sizeof ptr * 2];

	buf[0] = '*';

	const int len = 1 + u_to_hexstr(reinterpret_cast<uptr>(ptr), &buf[1]);

	x->_1vec(len, buf);
}

void output_buffer_src(
    output_buffer* x,  ///<      context.
    const char* path,  ///< [in] source file path.
    int line,          ///< [in] line number.
    const char* func)  ///< [in] function name. null available.
{
	iovec iov[5];

	iov[0].base = const_cast<char*>(path);
	iov[0].bytes = str_length(path);

	const char sep = ':';
	iov[1].base = const_cast<char*>(&sep);
	iov[1].bytes = 1;

	char line_buf[10];
	iov[2].bytes = u_to_decstr(line, line_buf);
	iov[2].base = line_buf;

	int iov_count;
	if (func) {
		iov[3].base = const_cast<char*>(&sep);
		iov[3].bytes = 1;

		iov[4].base = const_cast<char*>(func);
		iov[4].bytes = str_length(func);

		iov_count = 5;
	} else {
		iov_count = 3;
	}

	x->_vec(iov_count, iov);
}

void output_buffer_hexv(
    output_buffer* x,  ///< context.
    uptr bytes,        ///< [in] byte size of data.
    const void* data,  ///< [in] output data.
    int width,         ///< [in] see output_buffer::x().
    int columns,       ///< [in] see output_buffer::x().
    const char* summary) ///< [in] see output_buffer::x().
{
	const static char sep = ' ';
	const static char nl = '\n';

	const u8*       base = static_cast<const u8*>(data);

	const int index_width = up_div<u16>(find_last_setbit(bytes), 4);

	bool cpu_byte_order;
	if (width < 0) {
		width = -width;
		cpu_byte_order = false;
	} else {
		if (width == 0)
			width = sizeof (ucpu);
		cpu_byte_order = true;
	}

	if (summary) {
		(*x).c('[').str(summary).c(',').
		     p(data).c(',').
		     u(bytes).str("bytes,").
		     u(width).c('*').u(columns).
		     str(cpu_byte_order ? ",CPU" : ",RAW").str(" order,HEX]\n");
	}

	int col = 0;
	uptr off = 0;
	for (;;) {
		if (bytes - off < static_cast<uptr>(width))
			width = bytes - off;

		if (col == 0)
			(*x).x(off, index_width).c('|');

		for (int i = 0; i < width; ++i) {
			const u8* p;
			if (cpu_byte_order) {
				p = ARCH_IS_BE_LE(&base[off + i],
				                  &base[off + width - i - 1]);
			} else {
				p = &base[off + i];
			}
			char buf[2];
			u8_to_hexstr(*p, buf);
			x->_1vec(2, buf);
		}

		off += width;
		if (off == bytes)
			break;

		const char* p;
		if (++col < columns) {
			p = &sep;
		} else {
			p = &nl;
			col = 0;
		}
		x->_1vec(1, p);
	}
}

/// @brief hexdump with python format.
void output_buffer_hexv_py(
    output_buffer* x,  ///< context.
    uptr bytes,        ///< [in] byte size of data.
    const void* data,  ///< [in] output data.
    int width,         ///< [in] see output_buffer::x().
    int columns,       ///< [in] see output_buffer::x().
    const char* summary, ///< [in] see output_buffer::x().
    const char* suffix)  ///< [in] suffix of variable.
{
	const static char sep = ',';

	const u8*       base = static_cast<const u8*>(data);

	const int index_width = up_div<u16>(find_last_setbit(bytes), 4);

	bool cpu_byte_order;
	if (width < 0) {
		width = -width;
		cpu_byte_order = false;
	} else {
		if (width == 0)
			width = sizeof (ucpu);
		cpu_byte_order = true;
	}

	const char* suf = suffix ? suffix : "";

	x->str("### PYTHON STYLE HEX DUMP START ###\n");

	if (summary) {
		(*x).str("summary")(suf)("=\"").str(summary).c('\"').
		     str("\nadr")(suf)("=0x").x(reinterpret_cast<uptr>(data)).
		     str("\nbytes")(suf)("=").u(bytes).
		     str("\nwidth")(suf)(",columns")(suf)("=").
		         u(width).c(',').u(columns).
		     str("\ncpu_byte_order")(suf)("=").u(cpu_byte_order)();
	}

	(*x).str("data")(suf)("=[\n");

	int col = 0;
	uptr off = 0;
	uptr line_off = off;
	for (;;) {
		if (bytes - off < static_cast<uptr>(width))
			width = bytes - off;

		if (col == 0)
			line_off = off;

		x->_1vec(2, "0x");
		for (int i = 0; i < width; ++i) {
			const u8* p;
			if (cpu_byte_order) {
				p = ARCH_IS_BE_LE(&base[off + i],
				                  &base[off + width - i - 1]);
			} else {
				p = &base[off + i];
			}
			char buf[2];
			u8_to_hexstr(*p, buf);
			x->_1vec(2, buf);
		}

		off += width;
		if (off == bytes)
			break;

		x->_1vec(1, &sep);

		if (++col >= columns) {
			(*x).str("#").x(line_off, index_width).c('\n');
			col = 0;
		}
	}

	(*x).str("]#").x(line_off, index_width).
	     str("\n### PYTHON STYLE HEX DUMP END ###");
}

// printf semicompatible format processor definition.

namespace {

struct field_spec
{
	u8 flags;

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
    const char* field,
    std::va_list va,
    const char** end,
    field_spec* spec)
{
	spec->flags = 0;
	for (;; ++field) {
		if (*field == '-')
			spec->flags |= JUST_LEFT;
		else if (*field == '+')
			spec->flags |= SIGN_PLUS;
		else if (*field == '0')
			spec->flags |= PAD_ZERO;
		else if (*field == ' ')
			spec->flags |= SIGN_SPACE;
		else if (*field == '#')
			spec->flags |= SHAPE;
		else
			break;
	}

	if (*field == '*') {
		spec->width = va_arg(va, int);
		++field;
	} else {
		spec->width = str_to_u(10, field, &field);
	}

	if (*field == '.') {
		++field;
		if (*field == '*') {
			spec->precision = va_arg(va, int);
			++field;
		} else {
			spec->precision = str_to_u(10, field, &field);
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
		spec->flags |= UPPER;
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

void fmt_str(output_buffer* x, std::va_list va, const field_spec& spec)
{
	const char* str = va_arg(va, const char*);

	output_buffer_strf(x, str, spec.width, spec.precision, spec.flags);
}

void fmt_chr(output_buffer* x, std::va_list va, const field_spec& /*spec*/)
{
	const char c = va_arg(va, int);

	x->_1vec(1, &c);
}

void fmt_udec(output_buffer* x, std::va_list va, const field_spec& spec)
{
	const umax val =
	    spec.type == field_spec::TYPE_LONGLONG ?
	        va_arg(va, u64) :
	        va_arg(va, uint);

	output_buffer_uf(x, val, spec.width, spec.precision, spec.flags);
}

void fmt_sdec(output_buffer* x, std::va_list va, const field_spec& spec)
{
	const smax val =
	    spec.type == field_spec::TYPE_LONGLONG ?
	        va_arg(va, s64) :
	        va_arg(va, sint);

	output_buffer_sf(x, val, spec.width, spec.precision, spec.flags);
}

void fmt_oct(output_buffer* x, std::va_list va, const field_spec& spec)
{
	const umax val =
	    spec.type == field_spec::TYPE_LONGLONG ?
	        va_arg(va, u64) :
	        va_arg(va, uint);

	output_buffer_octf(x, val, spec.width, spec.precision, spec.flags);
}

void fmt_hex(output_buffer* x, std::va_list va, const field_spec& spec)
{
	const umax val =
	    spec.type == field_spec::TYPE_LONGLONG ?
	        va_arg(va, u64) :
		va_arg(va, uint);

	output_buffer_hexf(x, val, spec.width, spec.precision, spec.flags);
}

}  // namespace

void output_buffer_format(
    output_buffer* x,
    const char* format,
    std::va_list va)
{
	const char* fmt = format;

	const char* raw_out_pos = fmt;
	uptr raw_out_len = 0;

	while (*fmt) {
		if (*fmt == '%') {
			if (raw_out_len != 0) {
				x->_1vec(raw_out_len, raw_out_pos);
				raw_out_len = 0;
			}

			++fmt;

			const char* fmt_end;
			field_spec spec;
			decode_field(fmt, va, &fmt_end, &spec);

			switch (spec.style) {
			case 's':
				fmt_str(x, va, spec);
				break;

			case 'c':
				fmt_chr(x, va, spec);
				break;

			case 'u':
				fmt_udec(x, va, spec);
				break;

			case 'd':
				fmt_sdec(x, va, spec);
				break;

			case 'o':
				fmt_oct(x, va, spec);
				break;

			case 'p':
				spec.style = 'x';
				if (sizeof (cpu_word) == sizeof (u64))
				    spec.type = field_spec::TYPE_LONGLONG;
				fmt_hex(x, va, spec);
				break;

			case 'X':
			case 'x':
				fmt_hex(x, va, spec);
				break;

			case '%':
				x->_1vec(1, "%");
				break;

			default:
				x->_1vec(fmt_end - fmt+1, fmt-1);
				x->_flush();
				break;
			}

			fmt = fmt_end;
			raw_out_pos = fmt;
		} else {
			++raw_out_len;
			++fmt;
		}
	}

	if (raw_out_len != 0)
		x->_1vec(raw_out_len, raw_out_pos);
}

cause::type output_buffer_flush(output_buffer* x)
{
	return x->_flush();
}

// output_buffer

output_buffer::output_buffer(io_node* dest, io_node::offset off)
{
	info.destination = dest;
	info.dest_offset = off;
	info.buf_offset = 0;
}

void output_buffer::_vec(int iov_cnt, const iovec* iov)
{
	_flush();

	info.destination->write(&info.dest_offset, iov_cnt, iov);
}

void output_buffer::_1vec(uptr bytes, const void* data)
{
	const uptr buf_left = sizeof buffer - info.buf_offset;
	if (buf_left >= bytes) {
		// data が buffer に収まる場合は write() せずに
		// buffer に格納する。
		mem_copy(bytes, data, &buffer[info.buf_offset]);
		info.buf_offset += bytes;
	} else {
		// data が buffer に収まらない場合は buffer と data を
		// つないだ iovec で write() する。
		iovec iov[2];
		iov[0].base = buffer;
		iov[0].bytes = info.buf_offset;
		iov[1].base = const_cast<void*>(data);
		iov[1].bytes = bytes;

		const io_node::offset before_offset = info.dest_offset;
		info.destination->write(&info.dest_offset, 2, iov);

		const uptr write_bytes = info.dest_offset - before_offset;
		if (write_bytes != info.buf_offset + bytes)
		{
			// すべて出力できなかった場合は buffer を更新する。

			// 出力できなかった data のサイズ。
			const uptr data_left_bytes =
			    write_bytes > info.buf_offset ?
			        bytes - (write_bytes - info.buf_offset) :
			        bytes;

			if (write_bytes < info.buf_offset) {
				// 出力できなかった buffer の内容を
				// buffer の先頭に移動する。
				mem_move(info.buf_offset - write_bytes,
				         &buffer[write_bytes],
				         buffer);
				info.buf_offset -= write_bytes;
			}

			// 出力できなかった data の内容を buffer に格納する。
			const u8* data_left = static_cast<const u8*>(data)
			                    + bytes
			                    - data_left_bytes;
			append(data_left_bytes, data_left);
		} else {
			info.buf_offset = 0;
		}
	}
}

void output_buffer::_rep(int count, char c)
{
	while (count > 0) {
		if (info.buf_offset >= sizeof buffer)
			_flush();

		buffer[info.buf_offset] = c;

		++info.buf_offset;
		--count;
	}
}

cause::type output_buffer::_flush()
{
	if (info.buf_offset) {
		iovec iov;
		iov.base = buffer;
		iov.bytes = info.buf_offset;

		const io_node::offset before_offset = info.dest_offset;
		cause::type r =
		    info.destination->write(&info.dest_offset, 1, &iov);
		if (is_fail(r))
			return r;

		const uptr output_bytes = info.dest_offset - before_offset;
		if (output_bytes == 0)
			return r;
		if (output_bytes != info.buf_offset) {
			mem_move(info.buf_offset - output_bytes,
			         &buffer[output_bytes],
			         buffer);
			info.buf_offset -= output_bytes;

			return cause::OK;
		}

		info.buf_offset = 0;
	}

	return cause::OK;
}

uptr output_buffer::append(uptr bytes, const void* data)
{
	const uptr len =
	    min(static_cast<uptr>(sizeof buffer - info.buf_offset), bytes);

	mem_copy(len, data, &buffer[info.buf_offset]);

	info.buf_offset += len;

	return len;
}

