/// @file  log.hh
//
// (C) 2008-2011 KATO Takeshi
//

#ifndef INCLUDE_LOG_HH_
#define INCLUDE_LOG_HH_

#include "file.hh"


// @brief  Kernel log output destination.
class log_target
{
	void put_uhex(umax n, int bits);
	void put_uoct(umax n, int bits);
	void put_ubin(umax n, int bits);
	void put_udec(uptr n);

protected:
	typedef cause::stype (*write_function)
	    (log_target* self, const void* data, u32 bytes);
	write_function write_func;
	static cause::stype default_write_func(
	    log_target* self, const void* data, u32 bytes);

	void (*writec_func)(log_target* self, u8 c);
	static void default_writec_func(
	    log_target* self, u8 c);

protected:
	log_target() {}
	log_target(write_function wf) : write_func(wf) {}

public:
	log_target& operator () (char ch) { return c(ch); }
	log_target& operator () (const char* s) { return str(s); }
	log_target& operator () (const void* p) {
		return c('*').u(reinterpret_cast<u64>(p), 16);
	}
	log_target& operator () (const char* path, int line) {
		return str(path).c('(').udec(line).c(')');
	}
	log_target& operator () (const char* path, int line, const char* func) {
		return str(path).c('(').udec(line).str("):").str(func);
	}
	log_target& operator () () { return endl(); }

	log_target& c(char ch);
	log_target& str(const char* s);
	log_target& s(s8 n, int base=10);
	log_target& s(s16 n, int base=10);
	log_target& s(s32 n, int base=10);
	log_target& s(s64 n, int base=10);
	log_target& u(u8 n, int base=10);
	log_target& u(u16 n, int base=10);
	log_target& u(u32 n, int base=10);
	log_target& u(u64 n, int base=10);
	log_target& sdec(s64 n);
	log_target& udec(u64 n);
	log_target& u8hex(u8 n);
	log_target& u16hex(u16 n);
	log_target& u32hex(u32 n);
	log_target& u64hex(u64 n);
	log_target& endl() { return str("\r\n"); }
	log_target& write(const void* data, u32 bytes) {
		call_write(data, bytes);
		return *this;
	}

	void call_write(const void* data, u32 bytes) {
		write_func(this, data, bytes);
	}
};

// @brief  log to file_interface
class log_file : public log_target
{
	file* target;

	static cause::stype write(
	    log_target* self, const void* data, u32 bytes);

public:
	log_file() {}
	log_file(file* _file) : target(_file) {
		write_func = write;
	}

	void attach(file* _file) {
		target = _file;
		write_func = write;
	}

	file* get_file() { return target; }
};


void log_init(log_target* target);
log_target& log(u8 i=0);
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


#endif  // Include guard.
