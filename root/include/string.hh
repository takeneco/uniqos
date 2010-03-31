// @file   include/string.hh
// @author Kato Takeshi
// @brief  Memory ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _INCLUDE_STRING_HH_
#define _INCLUDE_STRING_HH_

#include "btypes.hh"


void MemoryMove(ucpu Bytes, const void* Src, void* Dest);
ucpu StringGetLengh(const char* Str);


#endif  // Include guard.
