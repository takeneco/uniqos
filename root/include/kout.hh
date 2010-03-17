// @file    include/kout.hh
// @author  Kato Takeshi
//
// (C) Kato Takeshi 2008-2010

#ifndef KOUT_HH_
#define KOUT_HH_

#include <cstddef>
#include "btypes.hh"


// @brief  Kernel message output destination.
//
class kern_output
{
	void putux(_umax n, int bits);

protected:
	kern_output() {}

public:
	virtual ~kern_output() {}

	kern_output* puts(const char* str);
	kern_output* putsd(_s64 n);
	kern_output* putud(_u64 n);
	kern_output* put8x(_u8 n);
	kern_output* put16x(_u16 n);
	kern_output* put32x(_u32 n);
	kern_output* put64x(_u64 n);

	virtual kern_output* putc(char c) =0;
	virtual void sync();
};


#endif  // Include guard.
