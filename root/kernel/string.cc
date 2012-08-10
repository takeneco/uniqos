/// @file   kernel/string.cc
/// @brief  Memory ops.
//
// (C) 2010-2012 KATO Takeshi
//

#include <string.hh>
#include <string.h>


int mem_compare(uptr bytes, const void* mem1, const void* mem2)
{
	const u8* m1 = static_cast<const u8*>(mem1);
	const u8* m2 = static_cast<const u8*>(mem2);

	for (uptr i = 0; i < bytes; i++) {
		u8 d = m1[i] - m2[i];
		if (d != 0)
			return d;
	}

	return 0;
}

void mem_move(uptr bytes, const void* src, void* dest)
{
	const char* s = static_cast<const char*>(src);
	char* d = static_cast<char*>(dest);

	if (d < s) {
		for (ucpu i = 0; i < bytes; i++)
			d[i] = s[i];
	} else {
		ucpu i = bytes;
		while (i--)
			d[i] = s[i];
	}
}

void mem_copy(uptr bytes, const void* src, void* dest)
{
	const char* s = static_cast<const char*>(src);
	char* d = static_cast<char*>(dest);

	for (uptr i = 0; i < bytes; ++i)
		d[i] = s[i];
}

void mem_fill(uptr bytes, u8 c, void* dest)
{
	u8* d = static_cast<u8*>(dest);

	for (uptr i = 0; i < bytes; i++)
		d[i] = c;
}

int str_length(const char* str)
{
	if (!str)
		return 0;

	int len = 0;
	while (str[len])
		len++;

	return len;
}

int str_compare(uptr max, const char* str1, const char* str2)
{
	for (uptr i = 0; i < max ; ++i) {
		if (str1[i] != str2[i])
			return str1[i] - str2[i];
		if (!str1[i])
			return 0;
	}

	return 0;
}

void str_copy(uptr max, const char* src, char* dest)
{
	for (uptr i = 0; i < max; ++i) {
		dest[i] = src[i];
		if (src[i])
			break;
	}
}

void str_concat(uptr max, const char* src, char* dest)
{
	while (*dest)
		dest++;

	for (uptr i = 0; ; ++i) {
		if (i >= max) {
			*dest = '\0';
			break;
		}
		if ((*dest++ = *src++) == '\0')
			break;
	}
}

extern "C" {

int memcmp(const void *s1, const void *s2, unsigned long n)
{
	return mem_compare(n, s1, s2);
}

void* memcpy(void* dest, const void *src, unsigned long n)
{
	mem_copy(n, src, dest);
	return dest;
}

void* memset(void* dest, int c, unsigned long n)
{
	mem_fill(n, c, dest);
	return dest;
}

unsigned long strlen(const char* s)
{
	return str_length(s);
}

int strcmp(const char *str1, const char *str2)
{
	return str_compare(0xffffffff, str1, str2);
}

char* strcat(char* dest, const char* src)
{
	str_concat(0xffffffff, src, dest);
	return dest;
}

char* strcpy(char* dest, const char* src)
{
	str_copy(0xffffffff, src, dest);
	return dest;
}

}  // extern "C"

