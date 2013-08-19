/// @file  basic_defs.hh

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef INCLUDE_BASIC_DEFS_HH_
#define INCLUDE_BASIC_DEFS_HH_


/// Disable default copy constructor and operator=.
/// @code
/// class example
/// {
///     DISALLOW_COPY_AND_ASSIGN(example);
/// public:
/// };
/// @endcode
#define DISALLOW_COPY_AND_ASSIGN(T)  T(const T&);void operator=(const T&)


#define num_of_array(array) (sizeof array / sizeof array[0])


#ifndef __has_builtin  // defined by clang
# define __has_builtin(x) 0
#endif  // __has_builtin

#if defined(__GNUC__) || __has_builtin(__builtin_expect)
# define LIKELY(x)   __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(x, 0)
#else  // !__builtin_expect
# define LIKELY(x)   (x)
# define UNLIKELY(x) (x)
#endif  // __builtin_expect

#ifdef ARCH_BE
# define ARCH_IS_BE_LE(BE_CODE, LE_CODE)  (BE_CODE)
#else   // ARCH_BE
# define ARCH_IS_BE_LE(BE_CODE, LE_CODE)  (LE_CODE)
#endif  // ARCH_BE


template<class xint> bool test_overlap(
    xint a1, xint a2, xint b1, xint b2, xint* r1, xint* r2)
{
	const xint p1 = max(a1, b1);
	const xint p2 = min(a2, b2);
	if (p1 > p2)
		return false;

	*r1 = p1;
	*r2 = p2;
	return true;
}


/// @brief 値が循環する整数型。
/// @tparam TYPE 基本整数型。u8, u16, ..., s8, s16, ... のいずれか。
//
/// 大小を比較する演算を定義する。
template<class TYPE>
class cycle_scalar
{
	typedef cycle_scalar<TYPE> own_t;
	typedef typename signed_of<TYPE>::t STYPE;

public:
	cycle_scalar() {}
	cycle_scalar(const own_t& x) : val(x.val) {}
	explicit cycle_scalar(TYPE x) : val(x) {}

	/// less than
	bool is_lt(const own_t& x) const {
		return static_cast<STYPE>(val) - static_cast<STYPE>(x.val) < 0;
	}
	/// less equal
	bool is_le(const own_t& x) const {
		return static_cast<STYPE>(val) - static_cast<STYPE>(x.val) <= 0;
	}
	/// greater than
	bool is_gt(const own_t& x) const {
		return static_cast<STYPE>(val) - static_cast<STYPE>(x.val) > 0;
	}
	/// greater equal
	bool is_ge(const own_t& x) const {
		return static_cast<STYPE>(val) - static_cast<STYPE>(x.val) >= 0;
	}

	own_t& operator = (const own_t& x) {
		val = x.val;
		return *this;
	}
	own_t& operator = (TYPE x) {
		val = x;
		return *this;
	}
	own_t operator + (const own_t& x) {
		return own_t(val + x.val);
	}
	own_t operator + (TYPE x) {
		return own_t(val + x);
	}
	bool operator < (const own_t& x) const {
		return is_lt(x);
	}
	bool operator <= (const own_t& x) const {
		return is_le(x);
	}
	bool operator > (const own_t& x) const {
		return is_gt(x);
	}
	bool operator >= (const own_t& x) const {
		return is_ge(x);
	}
	operator TYPE& () { return val; }

private:
	TYPE val;
};


#endif  // include guard

