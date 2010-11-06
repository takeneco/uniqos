/// @file  baseint.hh
/// @brief Builtin int types.
//
// (C) 2010 KATO Takeshi
//

#ifndef _ARCH_X86_64_INCLUDE_BASEINT_HH_
#define _ARCH_X86_64_INCLUDE_BASEINT_HH_


#if defined ARCH_W32 || defined ARCH_IA32

typedef   signed char      _s8;
typedef unsigned char      _u8;
typedef   signed short     _s16;
typedef unsigned short     _u16;
typedef   signed long      _s32;
typedef unsigned long      _u32;
typedef   signed long long _s64;
typedef unsigned long long _u64;
typedef   signed long long _smax;
typedef unsigned long long _umax;

typedef _u8  _ubyte;
typedef _s8  _sbyte;
typedef _u32 _ucpu;
typedef _s32 _scpu;
typedef _u32 _uptr;
typedef _s32 _sptr;

#  define _u64cast(n)  n ## ULL

#else  // !ARCH_W32 && !ARCH_IA32

typedef   signed char      _s8;
typedef unsigned char      _u8;
typedef   signed short     _s16;
typedef unsigned short     _u16;
typedef   signed int       _s32;
typedef unsigned int       _u32;
typedef   signed long      _s64;
typedef unsigned long      _u64;
typedef   signed long      _smax;
typedef unsigned long      _umax;

typedef _u8  _ubyte;
typedef _s8  _sbyte;
typedef _u64 _ucpu;
typedef _s64 _scpu;
typedef _u64 _uptr;
typedef _s64 _sptr;

#  define _u64cast(n)  n ## UL

#endif  // ARCH_IA32

#endif  // Include guards.
