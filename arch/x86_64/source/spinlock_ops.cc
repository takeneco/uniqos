/// @file  spinlock_ops.cc
/// @brief Spinlock operations.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2014 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <arch/spinlock_ops.hh>

#include <util/atomic.hh>


namespace {

inline s8 atomic8_xadd(s8 val, volatile s8* atom) {
	asm volatile ("lock xaddb %0, %1"
	              : "+r" (val), "+m" (*atom)
	              : : "memory", "cc");
	return val;
}
inline s16 atomic16_xadd(s16 val, volatile s16* atom) {
	asm volatile ("lock xaddw %0, %1"
	              : "+r" (val), "+m" (*atom)
	              : : "memory", "cc");
	return val;
}
inline s32 atomic32_xadd(s32 val, volatile s32* atom) {
	asm volatile ("lock xaddl %0, %1"
	              : "+r" (val), "+m" (*atom)
	              : : "memory", "cc");
	return val;
}
inline s64 atomic64_xadd(s64 val, volatile s64* atom) {
	asm volatile ("lock xaddq %0, %1"
	              : "+r" (val), "+m" (*atom)
	              : : "memory", "cc");
	return val;
}
inline s8  atomic_xadd(s8 val,  volatile s8* atom)  {
	return atomic8_xadd(val, atom);
}
inline s16 atomic_xadd(s16 val, volatile s16* atom) {
	return atomic16_xadd(val, atom);
}
inline s32 atomic_xadd(s32 val, volatile s32* atom) {
	return atomic32_xadd(val, atom);
}
inline s64 atomic_xadd(s64 val, volatile s64* atom) {
	return atomic64_xadd(val, atom);
}

}  // namespace


namespace arch {

bool spin_rwlock_ops::try_rlock()
{
	if (UNLIKELY(atomic_xadd(1, &atom) < 0)) {
		atomic_sub(1, &atom);
		return false;
	}

	return true;
}

bool spin_rwlock_ops::try_wlock()
{
	if (UNLIKELY(atomic_xadd(WLOCK, &atom) != 0)) {
		atomic_sub(WLOCK, &atom);
		return false;
	}

	return true;
}

void spin_rwlock_ops::un_rlock()
{
	atomic_sub(1, &atom);
}

void spin_rwlock_ops::un_wlock()
{
	atomic_sub(WLOCK, &atom);
}

}  // namespace arch

