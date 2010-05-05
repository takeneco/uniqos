// @file   include/string.hh
// @author Kato Takeshi
// @brief  Memory ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _INCLUDE_STRING_HH_
#define _INCLUDE_STRING_HH_

#include "btypes.hh"


void memory_move(const void* src, void* dest, ucpu bytes);
void memory_fill(u8 c, void* dest, ucpu bytes);
int string_get_length(const char* str);


#endif  // Include guard.
