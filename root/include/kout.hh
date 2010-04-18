// @file    include/kout.hh
// @author  Kato Takeshi
//
// (C) 2008-2010 Kato Takeshi.

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
	kern_output* PutC(char c);
	kern_output* PutStr(const char* str);
	kern_output* PutSDec(s64 n);
	kern_output* PutUDec(u64 n);
	kern_output* PutU8Hex(u8 n);
	kern_output* PutU16Hex(u16 n);
	kern_output* PutU32Hex(u32 n);
	kern_output* PutU64Hex(u64 n);

	virtual void Sync() {}
};


#endif  // Include guard.
