/// @file  baseint.hh
/// @brief local machine int types.
//
// (C) 2010 KATO Takeshi
//

#ifndef _TOOLS_BASEINT_HH_
#define _TOOLS_BASEINT_HH_

#include <stdint.h>


typedef  int8_t    _s8;
typedef uint8_t    _u8;
typedef  int16_t   _s16;
typedef uint16_t   _u16;
typedef  int32_t   _s32;
typedef uint32_t   _u32;
typedef  int64_t   _s64;
typedef uint64_t   _u64;
typedef  intmax_t  _smax;
typedef uintmax_t  _umax;

typedef  intptr_t  _scpu;
typedef uintptr_t  _ucpu;
typedef  intptr_t  _sptr;
typedef uintptr_t  _uptr;

#  define _u64cast(n)  n ## UL


#endif  // Include guards.
