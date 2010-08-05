// @file    include/kout.hh
// @author  Kato Takeshi
//
// (C) 2008-2010 Kato Takeshi

#ifndef _INCLUDE_KOUT_HH_
#define _INCLUDE_KOUT_HH_

#include "fileif.hh"


// @brief  Kernel message output destination.
//
class kern_output : public filenode_interface
{
	void putux(_umax n, int bits);

protected:
	kern_output() {}

public:
	kern_output* PutC(char c) { return put_c(c); }
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
