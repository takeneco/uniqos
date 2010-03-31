// @file   kernel/string.cc
// @author Kato Takeshi
// @brief  Memory ops.
//
// (C) 2010 Kato Takeshi.

#include "string.hh"


void MemoryMove(ucpu Bytes, const void* Src, void* Dest)
{
	const char* s = reinterpret_cast<const char*>(Src);
	char* d = reinterpret_cast<char*>(Dest);

	if (d < s) {
		for (ucpu i = 0; i < Bytes; i++)
			d[i] = s[i];
	} else {
		ucpu i = Bytes;
		while (i--)
			d[i] = s[i];
	}
}

ucpu StringGetLengh(const char* Str)
{
	if (!Str)
		return 0;

	ucpu len = 0;
	while (Str[len])
		len++;

	return len;
}
