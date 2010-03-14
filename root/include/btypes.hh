// @file    btypes.hh
// @author  Kato Takeshi
// @brief   共通で使う型・関数など。
//
// (C) Kato Takeshi 2008-2010

#ifndef BTYPES_HH_
#define BTYPES_HH_

#if defined ARCH_X86

#  define ARCH_LE

typedef   signed char      _s8;
typedef unsigned char      _u8;
typedef   signed short     _s16;
typedef unsigned short     _u16;
typedef   signed long      _s32;
typedef unsigned long      _u32;
typedef   signed long long _s64;
typedef unsigned long long _u64;

typedef _u32 _ucpu;
typedef _s32 _scpu;

#elif defined ARCH_X86_64

#  define ARCH_LE

typedef   signed char      _s8;
typedef unsigned char      _u8;
typedef   signed short     _s16;
typedef unsigned short     _u16;
typedef   signed int       _s32;
typedef unsigned int       _u32;
typedef   signed long      _s64;
typedef unsigned long      _u64;

typedef _u64 _ucpu;
typedef _s64 _scpu;

#endif  // ARCH_*

namespace {

inline _u16 _swap16(_u16 x) {
	return (x >> 8) | (x << 8);
}
inline void _split16(_u16 x, _u8* y1, _u8* y2) {
	*y1 = static_cast<_u8>(x >> 8);
	*y2 = static_cast<_u8>(x & 0xff);
}
inline _u16 _combine16(_u8 x1, _u8 x2) {
	return static_cast<_u16>(x1) << 8 | static_cast<_u16>(x2);
}
inline _u32 _swap32(_u32 x) {
	return x << 24 | x >> 24 |
		(x & 0x0000FF00) << 8 |
		(x & 0x00FF0000) >> 8;
}
inline void _split32(_u32 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4) {
	*y1 = static_cast<_u8>(x >> 24);
	*y2 = static_cast<_u8>(x >> 16);
	*y3 = static_cast<_u8>(x >> 8);
	*y4 = static_cast<_u8>(x);
}
inline _u32 _combine32(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return static_cast<_u32>(x1) << 24 | static_cast<_u32>(x2) << 16 |
		static_cast<_u32>(x3) << 8 | static_cast<_u32>(x4);
}
inline _u64 _swap64(_u64 x) {
	return x << 56 | x >> 56 |
		(x & 0x000000000000ff00L) << 40 |
		(x & 0x00ff000000000000L) >> 40 |
		(x & 0x0000000000ff0000L) << 24 |
		(x & 0x0000ff0000000000L) >> 24 |
		(x & 0x00000000ff000000L) <<  8 |
		(x & 0x000000ff00000000L) >>  8;
}
inline void _split64(_u64 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4,
	_u8* y5, _u8* y6, _u8* y7, _u8* y8) {
	*y1 = static_cast<_u8>(x >> 56);
	*y2 = static_cast<_u8>(x >> 48);
	*y3 = static_cast<_u8>(x >> 40);
	*y4 = static_cast<_u8>(x >> 32);
	*y5 = static_cast<_u8>(x >> 24);
	*y6 = static_cast<_u8>(x >> 16);
	*y7 = static_cast<_u8>(x >> 8);
	*y8 = static_cast<_u8>(x);
}
inline _u64 _combine64(_u8 x1, _u8 x2, _u8 x3, _u8 x4,
	_u8 x5, _u8 x6, _u8 x7, _u8 x8) {
	return
		static_cast<_u64>(x1) << 56 |
		static_cast<_u64>(x2) << 48 |
		static_cast<_u64>(x3) << 40 |
		static_cast<_u64>(x4) << 32 |
		static_cast<_u64>(x5) << 24 |
		static_cast<_u64>(x6) << 16 |
		static_cast<_u64>(x7) <<  8 |
		static_cast<_u64>(x8);
}

#if defined ARCH_LE

inline _u16 cpu_to_be16(_u16 x) {
	return _swap16(x);
}
inline _u16 be16_to_cpu(_u16 x) {
	return _swap16(x);
}
inline _u16 cpu_to_le16(_u16 x) {
	return x;
}
inline _u16 le16_to_cpu(_u16 x) {
	return x;
}
inline void cpu_to_be16(_u16 x, _u8* y1, _u8* y2) {
	_split16(x, y1, y2);
}
inline _u16 be16_to_cpu(_u8 x1, _u8 x2) {
	return _combine16(x1, x2);
}
inline void cpu_to_le16(_u16 x, _u8* y1, _u8* y2) {
	_split16(x, y2, y1);
}
inline _u16 le16_to_cpu(_u8 x1, _u8 x2) {
	return _combine16(x2, x1);
}
inline _u32 cpu_to_be32(_u32 x) {
	return _swap32(x);
}
inline _u32 be32_to_cpu(_u32 x) {
	return _swap32(x);
}
inline _u32 cpu_to_le32(_u32 x) {
	return x;
}
inline _u32 le32_to_cpu(_u32 x) {
	return x;
}
inline void cpu_to_be32(_u32 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4) {
	_split32(x, y1, y2, y3, y4);
}
inline _u32 be32_to_cpu(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return _combine32(x1, x2, x3, x4);
}
inline void cpu_to_le32(_u32 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4) {
	_split32(x, y4, y3, y2, y1);
}
inline _u32 le32_to_cpu(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return _combine32(x4, x3, x2, x1);
}
inline _u64 cpu_to_be64(_u64 x) {
	return _swap64(x);
}
inline _u64 be64_to_cpu(_u64 x) {
	return _swap64(x);
}
inline _u64 cpu_to_le64(_u64 x) {
	return x;
}
inline _u64 le64_to_cpu(_u64 x) {
	return x;
}
inline void cpu_to_be64(_u64 x, _u8* y0, _u8* y1, _u8* y2, _u8* y3,
	_u8* y4, _u8* y5, _u8* y6, _u8* y7) {
	_split64(x, y0, y1, y2, y3, y4, y5, y6, y7);
}
inline _u64 be64_to_cpu(_u8 x0, _u8 x1, _u8 x2, _u8 x3,
	_u8 x4, _u8 x5, _u8 x6, _u8 x7) {
	return _combine64(x0, x1, x2, x3, x4, x5, x6, x7);
}
inline void cpu_to_le64(_u64 x, _u8* y0, _u8* y1, _u8* y2, _u8* y3,
	_u8* y4, _u8* y5, _u8* y6, _u8* y7) {
	_split64(x, y7, y6, y5, y4, y3, y2, y1, y0);
}
inline _u64 le64_to_cpu(_u8 x0, _u8 x1, _u8 x2, _u8 x3,
	_u8 x4, _u8 x5, _u8 x6, _u8 x7) {
	return _combine64(x7, x6, x5, x4, x3, x2, x1, x0);
}

#elif defined ARCH_BE

#endif  // ARCH_LE or ARCH_BE

static inline _ucpu down_align(_ucpu base, _ucpu value) {
	return value & ~(base - 1);
}
static inline _ucpu up_align(_ucpu base, _ucpu value) {
	return (value + base - 1) & ~(base - 1);
}

}  // End of anonymous namespace

// NULL
const class {
public:
	template<class T> operator T*() const { return 0; }
//	template<class T, class U> operator T U::*() const { return 0; }
private:
	void operator&() const;
} null = {};

/*-------------------------------------------------------------------
 * リスト構造
 *-------------------------------------------------------------------*/

/**
 * @brief 双方向リストの要素。
 */
class bilist_elm
{
public:
	bilist_elm* prev;
	bilist_elm* next;

	bilist_elm();
};

inline bilist_elm::bilist_elm()
: prev(null), next(null)
{}

/**
 * @brief 双方向リスト。
 */
class bilist
{
public:
	bilist_elm* head;
	bilist_elm* tail;

	bilist();
};

inline bilist::bilist()
: head(null), tail(null)
{}

/*-------------------------------------------------------------------
 * エラーコード
 *-------------------------------------------------------------------*/

namespace result {
	// 0以上なら成功
	// 0未満なら失敗
	enum stype {
		OK = 0,
		FAIL = -1,
		NO_MEMORY = -2,
		INVALID_PARAMS = -3,
		INVALID_OPERATION = -4,
		UNKNOWN = -6,
	};

	typedef signed int type;
	inline bool isok(type t) { return t >= 0; }
	inline bool isfail(type t) { return t < 0; }
}

#define __TOSTR(x) #x
#define TOSTR(x) __TOSTR(x)

#endif  // BTYPES_HH_
