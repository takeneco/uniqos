/// @file  inttype.hh
/// @brief int types definition.
//
// (C) 2010-2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_BASEINT_HH_
#define ARCH_X86_64_INCLUDE_BASEINT_HH_


#if defined ARCH_W32 || defined ARCH_IA32

typedef   signed char      s8_;
typedef unsigned char      u8_;
typedef   signed short     s16_;
typedef unsigned short     u16_;
typedef   signed int       s32_;
typedef unsigned int       u32_;
typedef   signed long long s64_;
typedef unsigned long long u64_;

typedef u8_  ubyte_;
typedef s8_  sbyte_;
typedef u32_ ucpu_;
typedef s32_ scpu_;
typedef u32_ uptr_;
typedef s32_ sptr_;
typedef u16_ harf_uptr_;
typedef s16_ harf_sptr_;
typedef s64_ smax_;
typedef u64_ umax_;

const uptr_ UPTR_MAX = 0xffffffff;
const sptr_ SPTR_MAX = 0x7fffffff;

#  define suffix_s32(n)   n
#  define suffix_u32(n)   n ## U
#  define suffix_s64(n)   n ## LL
#  define suffix_u64(n)   n ## ULL
#  define suffix_sptr(n)  suffix_s32(n)
#  define suffix_uptr(n)  suffix_u32(n)

#else  // defined ARCH_W32 || defined ARCH_IA32

typedef   signed char      s8_;
typedef unsigned char      u8_;
typedef   signed short     s16_;
typedef unsigned short     u16_;
typedef   signed int       s32_;
typedef unsigned int       u32_;
typedef   signed long      s64_;
typedef unsigned long      u64_;

typedef u8_  ubyte_;
typedef s8_  sbyte_;
typedef u64_ ucpu_;
typedef s64_ scpu_;
typedef u64_ uptr_;
typedef s64_ sptr_;
typedef u32_ harf_uptr_;
typedef s32_ harf_sptr_;
typedef s64_ smax_;
typedef u64_ umax_;

const uptr_ UPTR_MAX = 0xffffffffffffffffUL;
const sptr_ SPTR_MAX = 0x7fffffffffffffffUL;

#  define suffix_s32(n)   n
#  define suffix_u32(n)   n ## U
#  define suffix_s64(n)   n ## L
#  define suffix_u64(n)   n ## UL
#  define suffix_sptr(n)  suffix_s64(n)
#  define suffix_uptr(n)  suffix_u64(n)

#endif  // defined ARCH_W32 || defined ARCH_IA32

#endif  // include guard
