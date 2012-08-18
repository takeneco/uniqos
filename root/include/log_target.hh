/// @file  log_target.hh
//
// (C) 2008-2012 KATO Takeshi
//

#ifndef INCLUDE_LOG_TARGET_HH_
#define INCLUDE_LOG_TARGET_HH_

#include <file.hh>


class log_target;

extern "C" {

void log_wr_str(
    log_target* x, const char* s);

void log_wr_u(
    log_target* x, u64 n, int base, int bits);

void log_wr_s(
    log_target* x, s64 n, int base, int bits);

void log_wr_p(
    log_target* x, const void* p);

void log_wr_src(
    log_target* x, const char* path, int line, const char* func);

void log_wr_bin(
    log_target* x, uptr bytes, const void* data);

}  // extern "C"


# include <cstdarg>

extern "C" void log_wr_fmt(log_target* x, const char* fmt, va_list va);


class log_target
{
	friend void log_wr_str(
	    log_target* x, const char* s);

	friend void log_wr_u(
	    log_target* x, u64 n, int base, int bits);

	friend void log_wr_s(
	    log_target* x, s64 n, int base, int bits);

	friend void log_wr_p(
	    log_target* x, const void* p);

	friend void log_wr_src(
	    log_target* x, const char* path, int line, const char* func);

	friend void log_wr_bin(
	    log_target* x, uptr bytes, const void* data);

public:
	log_target(file* _target) : target(_target) {}

	log_target& operator () (char ch) { return c(ch); }
	log_target& operator () (const char* s) { return str(s); }
	log_target& operator () (const void* p) {
		return this->p(p);
	}
	log_target& operator () (
	    const char* path, int line, const char* func = 0) {
		log_wr_src(this, path, line, func);
		return *this;
	}
	log_target& operator () () { return endl(); }

	log_target& write(const void* data, uptr bytes) {
		uptr wrote;
		iovec iov[1];
		iov[0].base = const_cast<void*>(data);
		iov[0].bytes = bytes;
		target->write(iov, 1, &wrote);
		return *this;
	}
	log_target& c(char ch) {
		write(&ch, 1);
		return *this;
	}
	log_target& str(const char* s) {
		log_wr_str(this, s);
		return *this;
	}
	log_target& u(u8 n, int base=10) {
		log_wr_u(this, n, base, 8);
		return *this;
	}
	log_target& u(u16 n, int base=10) {
		log_wr_u(this, n, base, 16);
		return *this;
	}
	log_target& u(u32 n, int base=10) {
		log_wr_u(this, n, base, 32);
		return *this;
	}
	log_target& u(u64 n, int base=10) {
		log_wr_u(this, n, base, 64);
		return *this;
	}
	log_target& u(s8 n, int base=10) {
		log_wr_u(this, n, base, 8);
		return *this;
	}
	log_target& u(s16 n, int base=10) {
		log_wr_u(this, n, base, 16);
		return *this;
	}
	log_target& u(s32 n, int base=10) {
		log_wr_u(this, n, base, 32);
		return *this;
	}
	log_target& u(s64 n, int base=10) {
		log_wr_u(this, n, base, 64);
		return *this;
	}
	log_target& s(u8 n, int base=10) {
		log_wr_s(this, n, base, 8);
		return *this;
	}
	log_target& s(u16 n, int base=10) {
		log_wr_s(this, n, base, 16);
		return *this;
	}
	log_target& s(u32 n, int base=10) {
		log_wr_s(this, n, base, 32);
		return *this;
	}
	log_target& s(u64 n, int base=10) {
		log_wr_s(this, n, base, 64);
		return *this;
	}
	log_target& s(s8 n, int base=10) {
		log_wr_s(this, n, base, 8);
		return *this;
	}
	log_target& s(s16 n, int base=10) {
		log_wr_s(this, n, base, 16);
		return *this;
	}
	log_target& s(s32 n, int base=10) {
		log_wr_s(this, n, base, 32);
		return *this;
	}
	log_target& s(s64 n, int base=10) {
		log_wr_s(this, n, base, 64);
		return *this;
	}
	log_target& p(const void* p) {
		log_wr_p(this, p);
		return *this;
	}
	log_target& endl() {
		write("\r\n", 2);
		return *this;
	}

	// src(SRCPOS)
#define SRCPOS  __FILE__,__LINE__,__func__
	log_target& src(const char* path, int line, const char* func) {
		log_wr_src(this, path, line, func);
		return *this;
	}
	log_target& bin(uptr bytes, const void* data) {
		log_wr_bin(this, bytes, data);
		return *this;
	}

private:
	void _wr_str(const char* s);
	void _wr_u(umax x, u8 base, int bits);
	void _wr_s(smax x, s8 base, int bits);
	void _wr_p(const void* p);
	void _wr_src(const char* path, int line, const char* func);
	void _wr_bin(uptr bytes, const void* data);

	friend void log_wr_fmt(
	    log_target* x, const char* fmt, va_list va);

public:
	log_target& format(const char* fmt, va_list va) {
		log_wr_fmt(this, fmt, va);
		return *this;
	}
	log_target& format(const char* fmt, ...) {
		va_list va;
		va_start(va, fmt);
		log_wr_fmt(this, fmt, va);
		va_end(va);
		return *this;
	}

private:
	void _wr_fmt(const char* fmt, va_list va);

	file* target;
};


#endif  // include guard

