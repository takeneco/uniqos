// @file   kernel/string.cc
// @author Kato Takeshi
// @brief  Memory ops.
//
// (C) 2010 Kato Takeshi.

#include "string.hh"


void memory_move(ucpu bytes, const void* src, void* dest)
{
	const char* s = reinterpret_cast<const char*>(src);
	char* d = reinterpret_cast<char*>(dest);

	if (d < s) {
		for (ucpu i = 0; i < bytes; i++)
			d[i] = s[i];
	} else {
		ucpu i = bytes;
		while (i--)
			d[i] = s[i];
	}
}

void memory_fill(ucpu bytes, u8 c, void* dest)
{
	u8* d = reinterpret_cast<u8*>(dest);

	for (ucpu i = 0; i < bytes; i++)
		d[i] = c;
}

int string_get_length(const char* str)
{
	if (!str)
		return 0;

	int len = 0;
	while (str[len])
		len++;

	return len;
}
