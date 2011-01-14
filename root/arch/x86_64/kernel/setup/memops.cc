// @file    arch/x86_64/kernel/setup/memops.cc
// @author  Kato Takeshi
//
// (C) 2009-2010 Kato Takeshi.

#include "mem.hh"


// @brief  Like memmove().
// @param dest Destination addr.
// @param src  Source addr.
// @param size Copy bytes.
// @return Always return dest.
void* memory_move(void* dest, const void* src, uptr size)
{
	char* d = reinterpret_cast<char*>(dest);
	const char* s = reinterpret_cast<const char*>(src);
	int n = static_cast<int>(size);

	if (dest < src) {
		for (int i = 0; i < n; i++) {
			d[i] = s[i];
		}
	}
	else {
		for (int i = n - 1; i >= 0; i--) {
			d[i] = s[i];
		}
	}

	return dest;
}

extern "C" void* memcpy(void* dest, void* src, uptr n)
{
	return memory_move(dest, src, n);
}

#ifdef __GNUC__

extern "C" void __cxa_pure_virtual()
{
}

#endif  // __GNUC__
