// @file   include/string.hh
// @brief  Memory ops.
//
// (C) 2010 KATO Takeshi
//

#ifndef INCLUDE_STRING_HH_
#define INCLUDE_STRING_HH_

#include "basic_types.hh"


void memory_move(const void* src, void* dest, ucpu bytes);
void memory_fill(u8 c, void* dest, ucpu bytes);
int string_get_length(const char* str);


#endif  // Include guard
