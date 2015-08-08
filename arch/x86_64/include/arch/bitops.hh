/// @file native_bitops.hh
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_NATIVE_BITOPS_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_BITOPS_HH_

#include <core/basic.hh>


namespace native {

/// @name bit scan forward/reverse instructions
/// _or0 の関数は、0 を入力すると -1 を返す。
/// @{

inline s16 bsfw(u16 data) {
	s16 index;
	asm ("bsfw %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s16 bsfw_or0(u16 data) {
	s16 index;
	asm ("bsfw %1, %0 \n"
	     "cmovzw %2, %0" : "=r" (index) : "rm" (data), "r" ((s16)-1));
	return index;
}
inline s16 bsfl(u32 data) {
	s32 index;
	asm ("bsfl %1, %0" : "=r" (index) : "rm" (data));
	return static_cast<s16>(index);
}
inline s16 bsfl_or0(u32 data) {
	s32 index;
	asm ("bsfl %1, %0 \n"
	     "cmovzl %2, %0" : "=r" (index) : "rm" (data), "r" ((s32)-1));
	return static_cast<s16>(index);
}
inline s16 bsfq(u64 data) {
	s64 index;
	asm ("bsfq %1, %0" : "=r" (index) : "rm" (data));
	return static_cast<s16>(index);
}
inline s16 bsfq_or0(u64 data) {
	s64 index;
	asm ("bsfq %1, %0 \n"
	     "cmovzq %2, %0" : "=r" (index) : "rm" (data), "r" ((s64)-1));
	return static_cast<s16>(index);
}
inline s16 bsrw(u16 data) {
	s16 index;
	asm ("bsrw %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s16 bsrw_or0(u16 data) {
	s16 index;
	asm ("bsrw %1, %0 \n"
	     "cmovzw %2, %0" : "=r" (index) : "rm" (data), "r" ((s16)-1));
	return index;
}
inline s16 bsrl(u32 data) {
	s32 index;
	asm ("bsrl %1, %0" : "=r" (index) : "rm" (data));
	return static_cast<s16>(index);
}
inline s16 bsrl_or0(u32 data) {
	s32 index;
	asm ("bsrl %1, %0 \n"
	     "cmovzl %2, %0" : "=r" (index) : "rm" (data), "r" ((s32)-1));
	return static_cast<s16>(index);
}
inline s16 bsrq(u64 data) {
	s64 index;
	asm ("bsrq %1, %0" : "=r" (index) : "rm" (data));
	return static_cast<s16>(index);
}
inline s16 bsrq_or0(u64 data) {
	s64 index;
	asm ("bsrq %1, %0 \n"
	     "cmovzq %2, %0" : "=r" (index) : "rm" (data), "r" ((s64)-1));
	return static_cast<s16>(index);
}

/// @}

}  // namespace native

namespace arch {

/// @name find_first_setbit_XX(), find_last_setbit_XX()
/// find_first_setbit(), find_last_setbit() から呼ばれる。
/// @{

inline s16 find_first_setbit_16(u16 data) { return native::bsfw_or0(data); }
inline s16 find_first_setbit_32(u32 data) { return native::bsfl_or0(data); }
inline s16 find_first_setbit_64(u64 data) { return native::bsfq_or0(data); }
inline s16 find_last_setbit_16(u16 data)  { return native::bsrw_or0(data); }
inline s16 find_last_setbit_32(u32 data)  { return native::bsrl_or0(data); }
inline s16 find_last_setbit_64(u64 data)  { return native::bsrq_or0(data); }

/// @}

}  // namespace arch


#endif  // include guard

