/**
 * @file    arch/x86/boot/phase4/util.cpp
 * @version 0.0.1
 * @date    2009-07-06
 * @author  Kato.T
 */
// (C) Kato.T 2009

#include <cstddef>

#include "phase4.hpp"

/**
 * memcpy()
 * memmove() のような動作が必要。
 */
void* memcpy(void* dest, const void* src, std::size_t n)
{
	char* d = reinterpret_cast<char*>(dest);
	const char* s = reinterpret_cast<const char*>(src);

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
