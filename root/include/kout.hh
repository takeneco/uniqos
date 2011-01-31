// @file  kout.hh
//
// (C) 2008-2011 KATO Takeshi
//

#ifndef _INCLUDE_KOUT_HH_
#define _INCLUDE_KOUT_HH_

#include "fileif.hh"


// @brief  Kernel message output destination.
class kout
{
	void put_ux(umax n, int bits);

protected:
	kout() {}

public:
	kout& operator () (char ch) { return c(ch); }
	kout& operator () (const char* s) { return str(s); }
	kout& operator () (const void* p) {
		return u64hex(reinterpret_cast<u64>(p));
	}
	kout& operator () (const char* path, int line) {
		return str(path).c('(').udec(line).c(')');
	}
	kout& operator () (const char* path, int line, const char* func) {
		return str(path).c('(').udec(line).str("):").str(func);
	}
	kout& operator () () { return endl(); }
	kout& endl() { return c('\n'); }

	kout& c(char ch);
	kout& str(const char* s);
	kout& sdec(s64 n);
	kout& udec(u64 n);
	kout& u8hex(u8 n);
	kout& u16hex(u16 n);
	kout& u32hex(u32 n);
	kout& u64hex(u64 n);

	virtual void write(char c);
};

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
