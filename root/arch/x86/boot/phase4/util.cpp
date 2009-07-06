/**
 * @file    arch/x86/boot/phase4/util.cpp
 * @version 0.0.1
 * @date    2009-07-06
 * @author  Kato.T
 */
// (C) Kato.T 2009

#include <cstddef>

/**
 * memcpy()
 * memmove() のような動作が必要。
 */
void* memcpy(void* dest, const void* src, std::size_t n)
{
	char* d = reinterpret_cast<char*>(dest);
	const char* s = reinterpret_cast<const char*>(src);

	for (int i = 0; i < n; i++) {
		*d++ = *s++;
	}

	return dest;
}
