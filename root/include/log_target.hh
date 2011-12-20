/// @file  log_target.hh
//
// (C) 2008-2011 KATO Takeshi
//

#ifndef INCLUDE_LOG_TARGET_HH_
#define INCLUDE_LOG_TARGET_HH_

#include "file.hh"


class log_target;

extern "C" {

void log_wr_str(log_target* x, const char* s);
void log_wr_u(log_target* x, u64 n, int base, int bits);
void log_wr_s(log_target* x, s64 n, int base, int bits);
void log_wr_p(log_target* x, const void* p);
void log_wr_src(log_target* x, const char* path, int line, const char* func);

}  // extern "C"

class log_target
{
	friend void log_wr_str(log_target* x, const char* s);
	friend void log_wr_u(log_target* x, u64 n, int base, int bits);
	friend void log_wr_s(log_target* x, s64 n, int base, int bits);
	friend void log_wr_p(log_target* x, const void* p);
	friend void log_wr_src(
	    log_target* x, const char* path, int line, const char* func);

protected:
	log_target() {}
	log_target(file* _target) : target(_target) {}

public:
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
		target->call_write(iov, 1, &wrote);
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
	log_target& src(const char* path, int line, const char* func) {
		log_wr_src(this, path, line, func);
		return *this;
	}

private:
	void _wr_str(const char* s);
	void _wr_u(umax x, u8 base, int bits);
	void _wr_s(smax x, s8 base, int bits);
	void _wr_p(const void* p);
	void _wr_src(const char* path, int line, const char* func);

	file* target;
};


void log_init(log_target* target);
//log_target& log(u8 i=0);
void log_set(u8 i, log_target* target);
void log_set(u8 i, bool mask);

//
class kern_output : public file
{
	void putux(umax n, int bits);

protected:
	kern_output() {}

public:
	kern_output* PutStr(const char* str) { return put_str(str); }
	kern_output* PutSDec(s64 n) { return put_sdec(n); }
	kern_output* PutUDec(u64 n) { return put_udec(n); }
	kern_output* PutU8Hex(u8 n) { return put_u8hex(n); }
	kern_output* PutU16Hex(u16 n) { return put_u16hex(n); }
	kern_output* PutU32Hex(u32 n) { return put_u32hex(n); }
	kern_output* PutU64Hex(u64 n) { return put_u64hex(n); }

	kern_output* put_c(char C);
	kern_output* put_str(const char* str);
	kern_output* put_sdec(s64 n);
	kern_output* put_udec(u64 n);
	kern_output* put_u8hex(u8 n);
	kern_output* put_u16hex(u16 n);
	kern_output* put_u32hex(u32 n);
	kern_output* put_u64hex(u64 n);

	kern_output* put_endl() { return put_c('\n'); }

	virtual void Sync() {}
};


#endif  // include guard
