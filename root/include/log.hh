/// @file  log.hh
//
// (C) 2008-2011 KATO Takeshi
//

#ifndef INCLUDE_LOG_HH_
#define INCLUDE_LOG_HH_

#include "fileif.hh"


// @brief  Kernel log output destination.
class kernel_log
{
	void put_uhex(umax n, int bits);
	void put_uoct(umax n, int bits);
	void put_ubin(umax n, int bits);
	void put_udec(umax n);

protected:
	typedef cause::stype (*write_function)
	    (kernel_log* self, const void* data, u32 bytes);
	write_function write_func;
	static cause::stype default_write_func(
	    kernel_log* self, const void* data, u32 bytes);

	void (*writec_func)(kernel_log* self, u8 c);
	static void default_writec_func(
	    kernel_log* self, u8 c);

protected:
	kernel_log() {}
	kernel_log(write_function wf) : write_func(wf) {}

public:
	kernel_log& operator () (char ch) { return c(ch); }
	kernel_log& operator () (const char* s) { return str(s); }
	kernel_log& operator () (const void* p) {
		return c('*').u(reinterpret_cast<u64>(p), 16);
	}
	kernel_log& operator () (const char* path, int line) {
		return str(path).c('(').udec(line).c(')');
	}
	kernel_log& operator () (const char* path, int line, const char* func) {
		return str(path).c('(').udec(line).str("):").str(func);
	}
	kernel_log& operator () () { return endl(); }

	kernel_log& c(char ch);
	kernel_log& str(const char* s);
	kernel_log& s(s8 n, int base=10);
	kernel_log& s(s16 n, int base=10);
	kernel_log& s(s32 n, int base=10);
	kernel_log& s(s64 n, int base=10);
	kernel_log& u(u8 n, int base=10);
	kernel_log& u(u16 n, int base=10);
	kernel_log& u(u32 n, int base=10);
	kernel_log& u(u64 n, int base=10);
	kernel_log& sdec(s64 n);
	kernel_log& udec(u64 n);
	kernel_log& u8hex(u8 n);
	kernel_log& u16hex(u16 n);
	kernel_log& u32hex(u32 n);
	kernel_log& u64hex(u64 n);
	kernel_log& endl() { return c('\n'); }
	kernel_log& write(const void* data, u32 bytes) {
		call_write(data, bytes);
		return *this;
	}

	void call_write(const void* data, u32 bytes) {
		write_func(this, data, bytes);
	}
};

// @brief  log to file_interface
class log_file : public kernel_log
{
	file_interface* file;

	static cause::stype write(
	    kernel_log* self, const void* data, u32 bytes);

public:
	log_file(file_interface* file_) : file(file_) {
		write_func = write;
	}
};


typedef kernel_log kout;

void log_init(kernel_log* target);
kernel_log& log(u8 i=0);
void log_set(u8 i, bool mask);

//
class kern_output : public filenode_interface
{
	void putux(_umax n, int bits);

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
