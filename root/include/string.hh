// @file   include/string.hh
// @brief  Memory ops.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef INCLUDE_STRING_HH_
#define INCLUDE_STRING_HH_

#include "basic_types.hh"


void mem_move(uptr bytes, const void* src, void* dest);
void mem_copy(uptr bytes, const void* src, void* dest);
void mem_fill(uptr bytes, u8 c, void* dest);
int string_get_length(const char* str);


#endif  // include guard
