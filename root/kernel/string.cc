/// @file   kernel/string.cc
/// @brief  Memory ops.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2012 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <string.hh>
#include <string.h>

#include <arch.hh>
#include <ctype.hh>


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

umax str_to_u(u8 base, const char* src, const char** end)
{
	umax result = 0;

	if (base == 0)
		base = 10;

	for (;;) {
		uint x;
		if (ctype::is_digit(*src))
			x = *src - '0';
		else if (ctype::is_lower(*src))
			x = *src - 'a' + 10;
		else if (ctype::is_upper(*src))
			x = *src - 'A' + 10;
		else
			break;

		if (x > base)
			break;

		result = result * base + x;

		++src;
	}

	if (end)
		*end = src;

	return result;
}

void str_to_upper(int length, char* str)
{
	for (int i = 0; i < length && str[i]; ++i) {
		if (ctype::is_lower(str[i]))
			str[i] += 'A' - 'a';
	}
}

namespace {

static const char base_chars[] = "0123456789abcdef";

}  // namespace

/// @retval output bytes.
int u_to_hexstr(
    umax n,                    /// [in] number.
    char s[sizeof (umax) * 2]) /// [out] output buffer. NOT nul terminate.
{
	int m = 0;
	for (int shift = sizeof n * arch::BITS_IN_BYTE - 4;
	     shift >= 0;
	     shift -= 4)
	{
		const int x = static_cast<u8>(n >> shift) & 0x0f;
		if (x == 0 && m == 0)
			continue;
		s[m++] = base_chars[x];
	}

	if (m == 0) {
		s[0] = '0';
		m = 1;
	}

	return m;
}

void u8_to_hexstr(
    u8 n,
    char s[2])
{
	s[0] = base_chars[(n >> 4) & 0xf];
	s[1] = base_chars[n & 0xf];
}

/// @retval  output bytes.
int u_to_octstr(
    umax n,                              /// [in] number.
    char s[(sizeof (umax) * 8 + 2) / 3]) /// [out] output buffer.
                                         ///       NOT nul terminate.
{
	int m = 0;
	for (int shift = sizeof n * arch::BITS_IN_BYTE - 3;
	     shift >= 0;
	     shift -= 3)
	{
		const int x = static_cast<u8>(n >> shift) & 0x07;
		if (x == 0 && m == 0)
			continue;
		s[m++] = base_chars[x];
	}

	if (m == 0) {
		s[0] = '0';
		m = 1;
	}

	return m;
}

/// @retval  output bytes.
int u_to_binstr(
    umax n,               /// [in] number.
    char s[sizeof n * 8]) /// [out] output buffer. NOT nul terminate.
{
	int m = 0;
	for (int shift = sizeof n * arch::BITS_IN_BYTE - 1;
	     shift >= 0;
	     shift -= 1)
	{
		const int x = static_cast<u8>(n >> shift) & 0x01;
		if (x == 0 && m == 0)
			continue;
		s[m++] = base_chars[x];
	}

	if (m == 0) {
		s[0] = '0';
		m = 1;
	}

	return m;
}

/// @retval  output bytes.
int u_to_decstr(
    umax n,               /// [in] number.
    char s[sizeof n * 3]) /// [out] output buffer. NOT nul terminate.
{
	int end = 0;
	for (umax _n = n; _n > 9; _n /= 10)
		++end;

	for (int i = 0; i <= end; ++i) {
		s[end - i] = base_chars[n % 10];
		n /= 10;
	}

	return end + 1;
}


extern "C" {

int memcmp(const void *s1, const void *s2, unsigned long n)
{
	return mem_compare(n, s1, s2);
}

void *memcpy(void *dest, const void *src, unsigned long n)
{
	mem_copy(n, src, dest);
	return dest;
}

void *memset(void *dest, int c, unsigned long n)
{
	mem_fill(n, c, dest);
	return dest;
}

unsigned long strlen(const char *s)
{
	return str_length(s);
}

int strcmp(const char *str1, const char *str2)
{
	return str_compare(0xffffffff, str1, str2);
}

char *strcpy(char *dest, const char *src)
{
	str_copy(0xffffffff, src, dest);
	return dest;
}

char *strncpy(char *dest, const char *src, unsigned long n)
{
	str_copy(n, src, dest);
	return dest;
}

char *strcat(char *dest, const char *src)
{
	str_concat(0xffffffff, src, dest);
	return dest;
}

unsigned long strtoul(const char* nptr, const char** endptr, int base)
{
	return str_to_u(base, nptr, endptr);
}

}  // extern "C"

