/// @file  output_buffer.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_OUTPUT_BUFFER_HH_
#define INCLUDE_OUTPUT_BUFFER_HH_

#include <cstdarg>
#include <io_node.hh>


class output_buffer;
void output_buffer_vec(output_buffer* x, int iov_cnt, const iovec* iov);
void output_buffer_1vec(output_buffer* x, uptr bytes, const void* data);
void output_buffer_str(output_buffer* x, const char* str);
void output_buffer_u(output_buffer* x, umax num);
void output_buffer_s(output_buffer* x, smax num);
void output_buffer_hex(output_buffer* x, umax num, int width);
void output_buffer_oct(output_buffer* x, umax num, int width);
void output_buffer_adr(output_buffer* x, const void* ptr);
void output_buffer_src(output_buffer* x,
                       const char* path, int line, const char* func);
void output_buffer_hexv(output_buffer* x, uptr bytes, const void* data,
                        int width, int cols, const char* summary);
void output_buffer_hexv_py(output_buffer* x, uptr bytes, const void* data,
                           int width, int cols, const char* summary,
                           const char* suffix);
void output_buffer_format(output_buffer* x, const char* format, va_list va);

cause::type output_buffer_flush(output_buffer* x);


class output_buffer
{
public:
	output_buffer(io_node* dest, io_node::offset off);
	~output_buffer() {
		if (info.buf_offset)
			output_buffer_flush(this);
	}

	io_node::offset get_offset() const { return info.dest_offset; }

	output_buffer& write(int iov_cnt, const iovec* iov) {
		output_buffer_vec(this, iov_cnt, iov);
		return *this;
	}
	output_buffer& write(uptr bytes, const void* data) {
		output_buffer_1vec(this, bytes, data);
		return *this;
	}

	output_buffer& c(char chr) {
		output_buffer_1vec(this, 1, &chr);
		return *this;
	}
	output_buffer& str(const char* str) {
		output_buffer_str(this, str);
		return *this;
	}
	template<class INT>
	output_buffer& u(INT n, int base=10) {
		if (base == 10) {
			//if (n < 0)
			//	output_buffer_s(this, n);
			//else
				output_buffer_u(this, static_cast<umax>(n));
		} else if (base == 8) {
			output_buffer_oct(this, n, sizeof n * 2);
		} else if (base == 16) {
			output_buffer_hex(this, n, sizeof n * 2);
		}
		return *this;
	}
	template<class INT>
	output_buffer& s(INT n) {
		output_buffer_s(this, static_cast<smax>(n));
		return *this;
	}
	template<class INT>
	output_buffer& x(INT n, int width=0) {
		output_buffer_hex(this, static_cast<umax>(n), width);
		return *this;
	}
	/// @brief hex dump.
	//
	/// data を width バイト単位に ' ' で区切って表示する。
	/// １行あたり width バイト単位を columns 回表示して改行する。
	/// width に負数を指定すると、CPUのバイトオータではなく、生のバイト
	/// 列として表示する。
	/// summary にNUL終端文字列を指定すると summary を含むヘッダを表示する。
	output_buffer& x(
	    uptr bytes,           ///< [in] byte size of output data.
	    const void* data,     ///< [in] output data.
	    int width = 1,        ///< [in] width.
	    int columns = 1,      ///< [in] columns.
	    const char* summary = 0) ///< [in] summary.
	{
		output_buffer_hexv(
		    this, bytes, data, width, columns, summary);
		return *this;
	}
	/// @brief python style hex dump.
	output_buffer& xpy(
	    uptr bytes,           ///< [in] byte size of output data.
	    const void* data,     ///< [in] output data.
	    int width = 1,        ///< [in] width.
	    int columns = 1,      ///< [in] columns.
	    const char* summary = 0, ///< [in] summary.
	    const char* suffix = 0)  ///< [in] suffix of variable.
	{
		output_buffer_hexv_py(
		    this, bytes, data, width, columns, summary, suffix);
		return *this;
	}
	template<class INT>
	output_buffer& o(INT n, int width=0) {
		output_buffer_oct(this, n, width);
		return *this;
	}
	output_buffer& p(const void* adr) {
		output_buffer_adr(this, adr);
		return *this;
	}
	output_buffer& endl() {
		c('\n');
		output_buffer_flush(this);
		return *this;
	}

	// src(SRCPOS)
#define SRCPOS  __FILE__,__LINE__,__func__
	output_buffer& src(const char* path, int line, const char* func=0) {
		output_buffer_src(this, path, line, func);
		return *this;
	}
	output_buffer& bin(uptr bytes, const void* data) {
		x(bytes, data, 1, 16, "bin() is DUPLICATED");
		return *this;
	}

	output_buffer& format(const char* fmt, std::va_list va) {
		output_buffer_format(this, fmt, va);
		return *this;
	}
	output_buffer& format(const char* fmt, ...) {
		va_list va;
		va_start(va, fmt);
		output_buffer_format(this, fmt, va);
		va_end(va);
		return *this;
	}

	output_buffer& operator () (char ch) { return c(ch); }
	output_buffer& operator () (const char* s) { return str(s); }
	output_buffer& operator () (const void* adr) { return p(adr); }
	output_buffer& operator () () { return endl(); }
	output_buffer& operator () (const char* path,int line,const char* func)
	    { return src(path, line, func); }

	cause::type flush() { return output_buffer_flush(this); }
	operator cause::type () { return flush(); }

public:
	void _vec(int iov_cnt, const iovec* iov);
	void _1vec(uptr bytes, const void* data);
	void _rep(int count, char c);

	cause::type _flush();

private:
	uptr append(uptr bytes, const void* data);

private:
	struct info_struct {
		io_node*        destination;
		io_node::offset dest_offset;
		u8              buf_offset;
	} info;

	u8 buffer[128 - sizeof(info_struct)];
};


#endif  // include guard

