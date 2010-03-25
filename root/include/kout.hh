// @file    include/kout.hh
// @author  Kato Takeshi
//
// (C) 2008-2010 Kato Takeshi

#ifndef _INCLUDE_KOUT_HH_
#define _INCLUDE_KOUT_HH_

#include "fileif.hh"


// @brief  Kernel message output destination.
//
class KernOutput : public FileNodeInterface
{
	void putux(_umax n, int bits);

protected:
	KernOutput() {}

public:
	KernOutput* PutC(char c);
	KernOutput* PutStr(const char* str);
	KernOutput* PutSDec(s64 n);
	KernOutput* PutUDec(u64 n);
	KernOutput* PutU8Hex(u8 n);
	KernOutput* PutU16Hex(u16 n);
	KernOutput* PutU32Hex(u32 n);
	KernOutput* PutU64Hex(u64 n);

	virtual void Sync() {}
};


#endif  // Include guard.
