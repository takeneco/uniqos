/// @file  basic-types.hh
/// @brief Common types, functions, etc.

//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
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

#ifndef CORE_BASIC_TYPES_HH_
#define CORE_BASIC_TYPES_HH_

#include <arch/inttype.hh>


#define U32(n)  suffix_u32(n)
#define S32(n)  suffix_s32(n)
#define U64(n)  suffix_u64(n)
#define S64(n)  suffix_s64(n)

typedef s8_        s8;
typedef u8_        u8;
typedef s16_       s16;
typedef u16_       u16;
typedef s32_       s32;
typedef u32_       u32;
typedef s64_       s64;
typedef u64_       u64;

typedef scpu_      scpu;
typedef ucpu_      ucpu;
typedef smax_      smax;
typedef umax_      umax;

typedef unsigned int uint;
typedef   signed int sint;

#if 16 < ARCH_ADR_BITS && ARCH_ADR_BITS <= 32
# define UPTR(n) suffix_u32(n)
# define SPTR(n) suffix_s32(n)

  typedef u32 uptr;
  typedef s32 sptr;

#elif 32 < ARCH_ADR_BITS && ARCH_ADR_BITS <= 64
# define UPTR(n) suffix_u64(n)
# define SPTR(n) suffix_s64(n)

  typedef u64 uptr;
  typedef s64 sptr;

#else
# error Unsupported ARCH_ADR_BITS value.

#endif  // ARCH_ADR_BITS


/// @struct harf_of
/// @brief  An integer type of harf width.
//
/// ex. harf_of<uptr>::t
/// - harf_of<u16>::t : u8
/// - harf_of<u32>::t : u16
/// - harf_of<u64>::t : u32
/// - harf_of<s16>::t : s8
/// - harf_of<s32>::t : s16
/// - harf_of<s64>::t : s32
template<class TYPE> struct harf_of;
template<> struct harf_of<u16> { typedef  u8 t; };
template<> struct harf_of<u32> { typedef u16 t; };
template<> struct harf_of<u64> { typedef u32 t; };
template<> struct harf_of<s16> { typedef  s8 t; };
template<> struct harf_of<s32> { typedef s16 t; };
template<> struct harf_of<s64> { typedef s32 t; };

/// @struct quarter_of
/// @brief  An integer type of quarter width.
/// - harf_of<u16>::t : u8
/// - harf_of<u32>::t : u8
/// - harf_of<u64>::t : u16
/// - harf_of<s16>::t : s8
/// - harf_of<s32>::t : s8
/// - harf_of<s64>::t : s16
template<class TYPE> struct quarter_of;
template<> struct quarter_of<u16> { typedef u8 t; };/// no type smaller than u8
template<> struct quarter_of<u32> { typedef u8 t; };
template<> struct quarter_of<u64> { typedef u16 t; };
template<> struct quarter_of<s16> { typedef s8 t; };/// no type smaller than s8
template<> struct quarter_of<s32> { typedef s8 t; };
template<> struct quarter_of<s64> { typedef s16 t; };

/// @struct unsigned_of
/// @brief Unsigned type corresponding to signed type
/// - unsigned_of<s8>::t  : u8
/// - unsigned_of<s16>::t : u16
/// - unsigned_of<s32>::t : u32
/// - unsigned_of<s64>::t : u64
template<class TYPE> struct unsigned_of;
template<> struct unsigned_of <s8> { typedef  u8 t; };
template<> struct unsigned_of<s16> { typedef u16 t; };
template<> struct unsigned_of<s32> { typedef u32 t; };
template<> struct unsigned_of<s64> { typedef u64 t; };

/// @struct signed_of
/// @brief Signed type corresponding to unsigned type
/// - signed_of<u8>::t  : s8
/// - signed_of<u16>::t : s16
/// - signed_of<u32>::t : s32
/// - signed_of<u64>::t : s64
template<class TYPE> struct signed_of;
template<> struct signed_of <u8> { typedef  s8 t; };
template<> struct signed_of<u16> { typedef s16 t; };
template<> struct signed_of<u32> { typedef s32 t; };
template<> struct signed_of<u64> { typedef s64 t; };


inline u16 swap16_(u16 x) {
	return (x >> 8) | (x << 8);
}
inline void split16_(u16 x, u8* y1, u8* y2) {
	*y1 = static_cast<u8>(x >> 8);
	*y2 = static_cast<u8>(x & 0xff);
}
inline u16 combine16_(u8 x1, u8 x2) {
	return static_cast<u16>(x1) << 8 | static_cast<u16>(x2);
}
inline u32 swap32_(u32 x) {
	return x << 24 | x >> 24 |
	    (x & U32(0x0000FF00)) << 8 |
	    (x & U32(0x00FF0000)) >> 8;
}
inline void split32_(u32 x, u8* y1, u8* y2, u8* y3, u8* y4) {
	*y1 = static_cast<u8>(x >> 24);
	*y2 = static_cast<u8>(x >> 16);
	*y3 = static_cast<u8>(x >> 8);
	*y4 = static_cast<u8>(x);
}
inline u32 combine32_(u8 x1, u8 x2, u8 x3, u8 x4) {
	return
	    static_cast<u32>(x1) << 24 |
	    static_cast<u32>(x2) << 16 |
	    static_cast<u32>(x3) << 8 |
	    static_cast<u32>(x4);
}
inline u64 swap64_(u64 x) {
	return x << 56 | x >> 56 |
	    (x & U64(0x000000000000ff00)) << 40 |
	    (x & U64(0x00ff000000000000)) >> 40 |
	    (x & U64(0x0000000000ff0000)) << 24 |
	    (x & U64(0x0000ff0000000000)) >> 24 |
	    (x & U64(0x00000000ff000000)) <<  8 |
	    (x & U64(0x000000ff00000000)) >>  8;
}
inline void split64_(u64 x, u8* y1, u8* y2, u8* y3, u8* y4,
	u8* y5, u8* y6, u8* y7, u8* y8) {
	*y1 = static_cast<u8>(x >> 56);
	*y2 = static_cast<u8>(x >> 48);
	*y3 = static_cast<u8>(x >> 40);
	*y4 = static_cast<u8>(x >> 32);
	*y5 = static_cast<u8>(x >> 24);
	*y6 = static_cast<u8>(x >> 16);
	*y7 = static_cast<u8>(x >> 8);
	*y8 = static_cast<u8>(x);
}
inline u64 combine64_(u8 x1, u8 x2, u8 x3, u8 x4,
	u8 x5, u8 x6, u8 x7, u8 x8) {
	return
	    static_cast<u64>(x1) << 56 |
	    static_cast<u64>(x2) << 48 |
	    static_cast<u64>(x3) << 40 |
	    static_cast<u64>(x4) << 32 |
	    static_cast<u64>(x5) << 24 |
	    static_cast<u64>(x6) << 16 |
	    static_cast<u64>(x7) <<  8 |
	    static_cast<u64>(x8);
}

#if defined ARCH_LE

template<class T> T cpu_to_be(T x);
template<> inline u16 cpu_to_be<u16>(u16 x) {
	return swap16_(x);
}
template<> inline u32 cpu_to_be<u32>(u32 x) {
	return swap32_(x);
}
template<> inline u64 cpu_to_be<u64>(u64 x) {
	return swap64_(x);
}

template<class T> T be_to_cpu(T x);
template<> inline u16 be_to_cpu<u16>(u16 x) {
	return swap16_(x);
}
template<> inline u32 be_to_cpu<u32>(u32 x) {
	return swap32_(x);
}
template<> inline u64 be_to_cpu<u64>(u64 x) {
	return swap64_(x);
}

template<class T> T cpu_to_le(T x);
template<> inline u16 cpu_to_le<u16>(u16 x) {
	return x;
}
template<> inline u32 cpu_to_le<u32>(u32 x) {
	return x;
}
template<> inline u64 cpu_to_le<u64>(u64 x) {
	return x;
}

template<class T> T le_to_cpu(T x);
template<> inline u16 le_to_cpu<u16>(u16 x) {
	return x;
}
template<> inline u32 le_to_cpu<u32>(u32 x) {
	return x;
}
template<> inline u64 le_to_cpu<u64>(u64 x) {
	return x;
}

inline u16 cpu_to_be16(u16 x) {
	return swap16_(x);
}
inline u16 be16_to_cpu(u16 x) {
	return swap16_(x);
}
inline u16 cpu_to_le16(u16 x) {
	return x;
}
inline u16 le16_to_cpu(u16 x) {
	return x;
}
inline void cpu_to_be16(u16 x, u8* y1, u8* y2) {
	split16_(x, y1, y2);
}
inline u16 be16_to_cpu(u8 x1, u8 x2) {
	return combine16_(x1, x2);
}
inline void cpu_to_le16(u16 x, u8* y1, u8* y2) {
	split16_(x, y2, y1);
}
inline u16 le16_to_cpu(u8 x1, u8 x2) {
	return combine16_(x2, x1);
}
inline u32 cpu_to_be32(u32 x) {
	return swap32_(x);
}
inline u32 be32_to_cpu(u32 x) {
	return swap32_(x);
}
inline u32 cpu_to_le32(u32 x) {
	return x;
}
inline u32 le32_to_cpu(u32 x) {
	return x;
}
inline void cpu_to_be32(u32 x, u8* y1, u8* y2, u8* y3, u8* y4) {
	split32_(x, y1, y2, y3, y4);
}
inline u32 be32_to_cpu(u8 x1, u8 x2, u8 x3, u8 x4) {
	return combine32_(x1, x2, x3, x4);
}
inline void cpu_to_le32(u32 x, u8* y1, u8* y2, u8* y3, u8* y4) {
	split32_(x, y4, y3, y2, y1);
}
inline u32 le32_to_cpu(u8 x1, u8 x2, u8 x3, u8 x4) {
	return combine32_(x4, x3, x2, x1);
}
inline u64 cpu_to_be64(u64 x) {
	return swap64_(x);
}
inline u64 be64_to_cpu(u64 x) {
	return swap64_(x);
}
inline u64 cpu_to_le64(u64 x) {
	return x;
}
inline u64 le64_to_cpu(u64 x) {
	return x;
}
inline void cpu_to_be64(u64 x, u8* y0, u8* y1, u8* y2, u8* y3,
	u8* y4, u8* y5, u8* y6, u8* y7) {
	split64_(x, y0, y1, y2, y3, y4, y5, y6, y7);
}
inline u64 be64_to_cpu(u8 x0, u8 x1, u8 x2, u8 x3,
	u8 x4, u8 x5, u8 x6, u8 x7) {
	return combine64_(x0, x1, x2, x3, x4, x5, x6, x7);
}
inline void cpu_to_le64(u64 x, u8* y0, u8* y1, u8* y2, u8* y3,
	u8* y4, u8* y5, u8* y6, u8* y7) {
	split64_(x, y7, y6, y5, y4, y3, y2, y1, y0);
}
inline u64 le64_to_cpu(u8 x0, u8 x1, u8 x2, u8 x3,
	u8 x4, u8 x5, u8 x6, u8 x7) {
	return combine64_(x7, x6, x5, x4, x3, x2, x1, x0);
}

#elif defined ARCH_BE

inline u16 cpu_to_be16(u16 x) {
	return x;
}
inline u16 be16_to_cpu(u16 x) {
	return x;
}
inline u16 cpu_to_le16(u16 x) {
	return swap16_(x);
}
inline u16 le16_to_cpu(u16 x) {
	return swap16_(x);
}
inline void cpu_to_be16(u16 x, u8* y1, u8* y2) {
	split16_(x, y2, y1);
}
inline u16 be16_to_cpu(u8 x1, u8 x2) {
	return combine16_(x2, x1);
}
inline void cpu_to_le16(u16 x, u8* y1, u8* y2) {
	split16_(x, y1, y2);
}
inline u16 le16_to_cpu(u8 x1, u8 x2) {
	return combine16_(x1, x2);
}

#endif  // ARCH_LE or ARCH_BE

// @param[in] align  Must be 2^n.
template <class uint_>
inline uint_ down_align(const uint_& value, const uint_& align) {
	return value & ~(align - 1);
}
// @param[in] align  Must be 2^n.
template <class uint_>
inline uint_ up_align(const uint_& value, const uint_& align) {
	return (value + align - 1) & ~(align - 1);
}
/// 余りを切り上げる割り算
/// val1 + val2 が uint_ の上限を越えてはいけない。
template <class uint_>
inline uint_ up_div(const uint_& val1, const uint_& val2) {
	return (val1 + val2 - 1) / val2;
}
/// 余りを切り上げる割り算
template <class uint_>
inline uint_ safe_up_div(const uint_& val1, const uint_& val2) {
	uint_ x = val1 / val2;
	return val1 % val2 == 0 ? x : x + 1;
}

template <class t_>
inline const t_& min(const t_& x, const t_& y) {
	return x <= y ? x : y;
}
template <class t_>
inline const t_& max(const t_& x, const t_& y) {
	return x >= y ? x : y;
}

template <class _T> int int_size_bits();
template <> inline int int_size_bits<u8>()  { return 3; }
template <> inline int int_size_bits<u16>() { return 4; }
template <> inline int int_size_bits<u32>() { return 5; }
template <> inline int int_size_bits<u64>() { return 6; }


/// @brief  Cause of error identification.
namespace cause
{
	enum type {
		OK = 0,
		FAIL = 1,

		/// メモリが不足している。
		NOMEM = 2,
		/// 引数が不正。
		BADARG = 3,
		/// IOが不正。
		BADIO = 4,
		/// 範囲外。
		OUTOFRANGE =5,
		/// データの終了に到達した。
		END = 6,
		// IO数の上限に達した。
		MAXIO = 7,
		/// デバイスが無い。
		NODEV = 10,
		/// No such file or directory
		NOENT = 11,
		/// Not directory
		NOTDIR = 12,
		/// Same name exist.
		EXIST = 13,

		/// メモリが割り当てられていない。
		NOT_ALLOCED,
		NOT_FOUND,
		INVALID_PARAMS,
		INVALID_OPERATION,
		INVALID_OBJECT,
		NOFUNC,
		/// バグ
		UNKNOWN = 0xffff,
	};

	typedef type t;
	typedef type stype;
	typedef u32 ftype;
	inline bool is_ok(type x) { return x == OK; }
	inline bool is_fail(type x) { return x != OK; }

	template<class T>
	struct pair
	{
		pair() :
			r(FAIL),
			_val(T())
		{}
		pair(type _r, const T& val) :
			r(_r),
			_val(val)
		{}

		pair<T>& operator = (const pair<T>& x) {
			r = x.r;
			_val = x._val;
			return *this;
		}

		operator T& () {
			return _val;
		}
		operator const T& () const {
			return _val;
		}
		T operator -> () {
			return _val;
		}

		void set_cause(type _r) { r = _r; }
		type get_cause() const { return r; }
		type cause() const { return r; }
		bool is_ok() const { return cause::is_ok(r); }
		bool is_fail() const { return cause::is_fail(r); }

		void set_data(const T& val) { _val = val; }
		T    get_data() { return _val; }
		const T& data() { return _val; }
		void set_value(T val) { _val = val; }
		T    get_value() { return _val; }
		const T& value() const { return _val; }

		type r;     ///< Result.
		T    _val;  ///< Additional result value.
	};

	template<class T> inline bool is_ok(T x) {
		return x.is_ok();
	}
	template<class T> inline bool is_fail(T x) {
		return x.is_fail();
	}
	template<class T> inline pair<T> make_pair(type r, T value) {
		return pair<T>(r, value);
	}

	struct _null_pair
	{
		_null_pair(type _r) : r(_r) {}
		template<class T> operator pair<T> () {
			return pair<T>(r, nullptr);
		}
		type r;
	};
	inline _null_pair null_pair(type r) {
		return _null_pair(r);
	}

	struct _zero_pair
	{
		_zero_pair(type _r) : r(_r) {}
		template<class T> operator pair<T> () {
			return pair<T>(r, 0);
		}
		type r;
	};
	inline _zero_pair zero_pair(type r) {
		return _zero_pair(r);
	}
}

/// @brief アドレス範囲
struct adr_range
{
	uptr low;
	uptr high;

	adr_range() {}
	adr_range(const adr_range& ar) :
		low(ar.low),
		high(ar.high)
	{}

	uptr low_adr() const { return low; }
	uptr high_adr() const { return high; }
	uptr bytes() const { return high - low + 1; }

	void set(const adr_range& ar) {
		low = ar.low;
		high = ar.high;
	}
	adr_range& operator = (const adr_range& ar) {
		set(ar);
		return *this;
	}

	void set_ab(uptr adr, uptr bytes) {
		low = adr;
		high = adr + bytes - 1;
	}
	void set_lh(uptr low_adr, uptr high_adr) {
		low = low_adr;
		high = high_adr;
	}

	static adr_range gen_ab(uptr adr, uptr bytes) {
		adr_range r;
		r.low = adr;
		r.high = adr + bytes - 1;
		return r;
	}
	static adr_range gen_lh(uptr low_adr, uptr high_adr) {
		adr_range r;
		r.low = low_adr;
		r.high = high_adr;
		return r;
	}

	/// @return adr が adr_range の範囲に含まれれば true を返す。
	///         そうでなければ false を返す。
	bool test(uptr adr) {
		return low <= adr && adr <= high;
	}
};

/**
 * @brief  Log information.
 */
/*
namespace log
{
	enum level {
		EMERGENCY = 0,
		ALERT,
		CRITICAL,
		ERROR,
		NOTICE,
		INFO,
		DEBUG,
	};

	typedef u32 type;
	inline bool IsHeavy(type x, type base) { return (x & 0x0f) <= base; }
	inline bool IsLight(type x, type base) { return (x & 0x0f) >= base; }
}
*/


#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)


#endif  // CORE_BASIC_TYPES_HH_

