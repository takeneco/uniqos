// @file   include/string.hh
// @author Kato Takeshi
// @brief  Memory ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _INCLUDE_STRING_HH_
#define _INCLUDE_STRING_HH_

#include "btypes.hh"


void memory_move(ucpu bytes, const void* src, void* dest);
void memory_fill(ucpu bytes, u8 c, void* dest);
int string_get_length(const char* str);


#endif  // Include guard.
