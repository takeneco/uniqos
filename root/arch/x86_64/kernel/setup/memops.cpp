/**
 * @file    arch/x86_64/kernel/setup/memops.cpp
 * @version 0.0.0.1
 * @author  Kato.T
 */
// (C) Kato.T 2009-2010

#include <cstddef>

#include "mem.hpp"

/**
 * @brief  Like memmove().
 * @param dest Destination addr.
 * @param src  Source addr.
 * @param size Copy bytes.
 * @return Always return dest.
 */
void* memory_move(void* dest, const void* src, std::size_t size)
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

#ifdef __GNUC__

extern "C" void __cxa_pure_virtual()
{
}

#endif  // __GNUC__
