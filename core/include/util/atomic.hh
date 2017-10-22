/// @file util/atomic.hh

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

#ifndef UTIL_ATOMIC_HH_
#define UTIL_ATOMIC_HH_

#include <arch/atomic_ops.hh>


namespace arch {

// atomic_exchange
inline u8 atomic_exchange(u8 new_val, volatile u8* atom) {
	return atomic8_exchange(new_val, atom);
}
inline u16 atomic_exchange(u16 new_val, volatile u16* atom) {
	return atomic16_exchange(new_val, atom);
}
inline u32 atomic_exchange(u32 new_val, volatile u32* atom) {
	return atomic32_exchange(new_val, atom);
}
inline u64 atomic_exchange(u64 new_val, volatile u64* atom) {
	return atomic64_exchange(new_val, atom);
}
inline s8 atomic_exchange(s8 new_val, volatile s8* atom) {
	return atomic8_exchange(new_val,
	                        reinterpret_cast<volatile u8*>(atom));
}
inline s16 atomic_exchange(s16 new_val, volatile s16* atom) {
	return atomic16_exchange(new_val,
	                         reinterpret_cast<volatile u16*>(atom));
}
inline s32 atomic_exchange(s32 new_val, volatile s32* atom) {
	return atomic32_exchange(new_val,
	                         reinterpret_cast<volatile u32*>(atom));
}
inline s64 atomic_exchange(s64 new_val, volatile s64* atom) {
	return atomic64_exchange(new_val,
	                         reinterpret_cast<volatile u64*>(atom));
}

// atomic_compare_exchange
inline
u8 atomic_compare_exchange(u8 old_val, u8 new_val, volatile u8* atom) {
	return atomic8_compare_exchange(old_val, new_val, atom);
}
inline
u16 atomic_compare_exchange(u16 old_val, u16 new_val, volatile u16* atom) {
	return atomic16_compare_exchange(old_val, new_val, atom);
}
inline
u32 atomic_compare_exchange(u32 old_val, u32 new_val, volatile u32* atom) {
	return atomic32_compare_exchange(old_val, new_val, atom);
}
inline
u64 atomic_compare_exchange(u64 old_val, u64 new_val, volatile u64* atom) {
	return atomic64_compare_exchange(old_val, new_val, atom);
}
inline
s8 atomic_compare_exchange(s8 old_val, s8 new_val, volatile s8* atom) {
	return atomic8_compare_exchange(
	    old_val, new_val, reinterpret_cast<volatile u8*>(atom));
}
inline
s16 atomic_compare_exchange(s16 old_val, s16 new_val, volatile s16* atom) {
	return atomic16_compare_exchange(
	    old_val, new_val, reinterpret_cast<volatile u16*>(atom));
}
inline
s32 atomic_compare_exchange(s32 old_val, s32 new_val, volatile s32* atom) {
	return atomic32_compare_exchange(
	    old_val, new_val, reinterpret_cast<volatile u32*>(atom));
}
inline
s64 atomic_compare_exchange(s64 old_val, s64 new_val, volatile s64* atom) {
	return atomic64_compare_exchange(
	    old_val, new_val, reinterpret_cast<volatile u64*>(atom));
}

// atomic_add
inline void atomic_add(u8 add_val, volatile u8* atom) {
	atomic8_add(add_val, atom);
}
inline void atomic_add(u16 add_val, volatile u16* atom) {
	atomic16_add(add_val, atom);
}
inline void atomic_add(u32 add_val, volatile u32* atom) {
	atomic32_add(add_val, atom);
}
inline void atomic_add(u64 add_val, volatile u64* atom) {
	atomic64_add(add_val, atom);
}
inline void atomic_add(s8 add_val, volatile s8* atom) {
	atomic8_add(add_val, reinterpret_cast<volatile u8*>(atom));
}
inline void atomic_add(s16 add_val, volatile s16* atom) {
	atomic16_add(add_val, reinterpret_cast<volatile u16*>(atom));
}
inline void atomic_add(s32 add_val, volatile s32* atom) {
	atomic32_add(add_val, reinterpret_cast<volatile u32*>(atom));
}
inline void atomic_add(s64 add_val, volatile s64* atom) {
	atomic64_add(add_val, reinterpret_cast<volatile u64*>(atom));
}

// atomic_sub
inline void atomic_sub(u8 sub_val, volatile u8* atom) {
	atomic8_sub(sub_val, atom);
}
inline void atomic_sub(u16 sub_val, volatile u16* atom) {
	atomic16_sub(sub_val, atom);
}
inline void atomic_sub(u32 sub_val, volatile u32* atom) {
	atomic32_sub(sub_val, atom);
}
inline void atomic_sub(u64 sub_val, volatile u64* atom) {
	atomic64_sub(sub_val, atom);
}
inline void atomic_sub(s8 sub_val, volatile s8* atom) {
	atomic8_sub(sub_val, reinterpret_cast<volatile u8*>(atom));
}
inline void atomic_sub(s16 sub_val, volatile s16* atom) {
	atomic16_sub(sub_val, reinterpret_cast<volatile u16*>(atom));
}
inline void atomic_sub(s32 sub_val, volatile s32* atom) {
	atomic32_sub(sub_val, reinterpret_cast<volatile u32*>(atom));
}
inline void atomic_sub(s64 sub_val, volatile s64* atom) {
	atomic64_sub(sub_val, reinterpret_cast<volatile u64*>(atom));
}

// atomic_sub_and_test0
inline u8 atomic_sub_and_test0(u8 sub_val, volatile u8* atom) {
	return atomic8_sub_and_test0(sub_val, atom);
}
inline u8 atomic_sub_and_test0(u16 sub_val, volatile u16* atom) {
	return atomic16_sub_and_test0(sub_val, atom);
}
inline u8 atomic_sub_and_test0(u32 sub_val, volatile u32* atom) {
	return atomic32_sub_and_test0(sub_val, atom);
}
inline u8 atomic_sub_and_test0(u64 sub_val, volatile u64* atom) {
	return atomic64_sub_and_test0(sub_val, atom);
}

}  // namespace arch


template<class T>
class atomic
{
public:
	typedef T type;

	atomic() {}
	atomic(T v) : val(v) {}

	void store(T v) {
		val = v;
	}
	T load() const {
		return val;
	}
	T exchange(T v) {
		return arch::atomic_exchange(v, &val);
	}
	T compare_exchange(T _old, T _new) {
		return arch::atomic_compare_exchange(_old, _new, &val);
	}
	void add(T v) {
		arch::atomic_add(v, &val);
	}
	void sub(T v) {
		arch::atomic_sub(v, &val);
	}
	void inc() {
		arch::atomic_add(1, &val);
	}
	void dec() {
		arch::atomic_sub(1, &val);
	}
	void dec_and_test0() {
		arch::atomic_sub_and_test0(1, &val);
	}

private:
	volatile T val;
};


#endif  // UTIL_ATOMIC_HH_

