/// @file atomic.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_ATOMIC_HH_
#define INCLUDE_ATOMIC_HH_

#include <atomic_ops.hh>


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
	atomic8_add(sub_val, atom);
}
inline void atomic_sub(u16 sub_val, volatile u16* atom) {
	atomic16_add(sub_val, atom);
}
inline void atomic_sub(u32 sub_val, volatile u32* atom) {
	atomic32_add(sub_val, atom);
}
inline void atomic_sub(u64 sub_val, volatile u64* atom) {
	atomic64_add(sub_val, atom);
}
inline void atomic_sub(s8 sub_val, volatile s8* atom) {
	atomic8_add(sub_val, reinterpret_cast<volatile u8*>(atom));
}
inline void atomic_sub(s16 sub_val, volatile s16* atom) {
	atomic16_add(sub_val, reinterpret_cast<volatile u16*>(atom));
}
inline void atomic_sub(s32 sub_val, volatile s32* atom) {
	atomic32_add(sub_val, reinterpret_cast<volatile u32*>(atom));
}
inline void atomic_sub(s64 sub_val, volatile s64* atom) {
	atomic64_add(sub_val, reinterpret_cast<volatile u64*>(atom));
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

private:
	volatile T val;
};


#endif  // include guard

