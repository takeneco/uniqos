/// @file atomic_ops.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ARCH_ATOMIC_OPS_HH_
#define ARCH_ATOMIC_OPS_HH_

#include <core/basic.hh>


namespace arch {

// atomic_exchange
inline u8 atomic8_exchange(u8 new_val, volatile u8* atom) {
	u8 r;
	asm volatile ("xchgb %0, %1"
	              : "=q" (r), "+m" (*atom)
	              : "0" (new_val)
	              : "memory");
	return r;
}
inline u16 atomic16_exchange(u16 new_val, volatile u16* atom) {
	u16 r;
	asm volatile ("xchgw %0, %1"
	              : "=r" (r), "+m" (*atom)
	              : "0" (new_val)
	              : "memory");
	return r;
}
inline u32 atomic32_exchange(u32 new_val, volatile u32* atom) {
	u32 r;
	asm volatile ("xchgl %0, %1"
	              : "=r" (r), "+m" (*atom)
	              : "0" (new_val)
		      : "memory");
	return r;
}
inline u64 atomic64_exchange(u64 new_val, volatile u64* atom) {
	u64 r;
	asm volatile ("xchgq %0, %1"
	              : "=r" (r), "+m" (*atom)
	              : "0" (new_val)
		      : "memory");
	return r;
}

// atomic_compare_exchange
inline
u8 atomic8_compare_exchange(u8 old_val, u8 new_val, volatile u8* atom) {
	u8 r;
	asm volatile ("lock cmpxchgb %2, %1"
	              : "=a" (r), "+m" (*atom)
	              : "q" (new_val), "0" (old_val)
	              : "memory");
	return r;
}
inline
u16 atomic16_compare_exchange(u16 old_val, u16 new_val, volatile u16* atom) {
	u16 r;
	asm volatile ("lock cmpxchgw %2, %1"
	              : "=a" (r), "+m" (*atom)
	              : "r" (new_val), "0" (old_val)
	              : "memory");
	return r;
}
inline
u32 atomic32_compare_exchange(u32 old_val, u32 new_val, volatile u32* atom) {
	u32 r;
	asm volatile ("lock cmpxchgl %2, %1"
	              : "=a" (r), "+m" (*atom)
	              : "r" (new_val), "0" (old_val)
	              : "memory");
	return r;
}
inline
u64 atomic64_compare_exchange(u64 old_val, u64 new_val, volatile u64* atom) {
	u64 r;
	asm volatile ("lock cmpxchgq %2, %1"
	              : "=a" (r), "+m" (*atom)
	              : "r" (new_val), "0" (old_val)
	              : "memory");
	return r;
}

// atomic_add
inline void atomic8_add(u8 add_val, volatile u8* atom) {
	asm volatile ("lock addb %1, %0"
	              : "+m" (*atom)
	              : "iq" (add_val));
}
inline void atomic16_add(u16 add_val, volatile u16* atom) {
	asm volatile ("lock addw %1, %0"
	              : "+m" (*atom)
	              : "ir" (add_val));
}
inline void atomic32_add(u32 add_val, volatile u32* atom) {
	asm volatile ("lock addl %1, %0"
	              : "+m" (*atom)
	              : "ir" (add_val));
}
inline void atomic64_add(u64 add_val, volatile u64* atom) {
	asm volatile ("lock addq %1, %0"
	              : "+m" (*atom)
	              : "ir" (add_val));
}

// atomic_sub
inline void atomic8_sub(u8 sub_val, volatile u8* atom) {
	asm volatile ("lock subb %1, %0"
	              : "+m" (*atom)
	              : "iq" (sub_val));
}
inline void atomic16_sub(u16 sub_val, volatile u16* atom) {
	asm volatile ("lock subw %1, %0"
	              : "+m" (*atom)
	              : "ir" (sub_val));
}
inline void atomic32_sub(u32 sub_val, volatile u32* atom) {
	asm volatile ("lock subl %1, %0"
	              : "+m" (*atom)
	              : "ir" (sub_val));
}
inline void atomic64_sub(u64 sub_val, volatile u64* atom) {
	asm volatile ("lock subq %1, %0"
	              : "+m" (*atom)
	              : "ir" (sub_val));
}

// atomic_sub_and_test0
inline int atomic8_sub_and_test0(u8 sub_val, volatile u8* atom) {
	u8 r;
	asm volatile ("lock subb %2, %0; setz %1"
	              : "+m" (*atom), "=qm" (r)
	              : "ir" (sub_val)
	              : "memory");
	return r;
}
inline int atomic16_sub_and_test0(u16 sub_val, volatile u16* atom) {
	u8 r;
	asm volatile ("lock subw %2, %0; setz %1"
	              : "+m" (*atom), "=qm" (r)
	              : "ir" (sub_val)
	              : "memory");
	return r;
}
inline int atomic32_sub_and_test0(u32 sub_val, volatile u32* atom) {
	u8 r;
	asm volatile ("lock subl %2, %0; setz %1"
	              : "+m" (*atom), "=qm" (r)
	              : "ir" (sub_val)
	              : "memory");
	return r;
}
inline int atomic64_sub_and_test0(u64 sub_val, volatile u64* atom) {
	u8 r;
	asm volatile ("lock subq %2, %0; setz %1"
	              : "+m" (*atom), "=qm" (r)
	              : "ir" (sub_val)
	              : "memory");
	return r;
}

}  // namespace arch


#endif  // ARCH_ATOMIC_OPS_HH_

