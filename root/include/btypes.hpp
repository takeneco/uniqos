/**
 * @file    btypes.hpp
 * @version 0.0.9
 * @date    2009-08-02
 * @author  Kato.T
 * @brief   共通で使う型・関数など。
 */
// (C) Kato.T 2008-2009

#ifndef BTYPES_HPP
#define BTYPES_HPP

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

static inline _u16 _swap16(_u16 x) {
	return (x >> 8) | (x << 8);
}
static inline void _split16(_u16 x, _u8* y1, _u8* y2) {
	*y1 = static_cast<_u8>(x >> 8);
	*y2 = static_cast<_u8>(x & 0xff);
}
static inline _u16 _combine16(_u8 x1, _u8 x2) {
	return static_cast<_u16>(x1) << 8 | static_cast<_u16>(x2);
}
static inline _u32 _swap32(_u32 x) {
	return x << 24 | x >> 24 |
		(x & 0x0000FF00UL) << 8 |
		(x & 0x00FF0000UL) >> 8;
}
static inline void _split32(_u32 x, _u8* y1, _u8* y2, _u8 *y3, _u8 *y4) {
	*y1 = static_cast<_u8>(x >> 24);
	*y2 = static_cast<_u8>(x >> 16);
	*y3 = static_cast<_u8>(x >> 8);
	*y4 = static_cast<_u8>(x);
}
static inline _u32 _combine32(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return static_cast<_u32>(x1) << 24 | static_cast<_u32>(x2) << 16 |
		static_cast<_u32>(x3) << 8 | static_cast<_u32>(x4);
}

#if defined ARCH_LE

static inline _u16 cpu_to_be16(_u16 x) {
	return _swap16(x);
}
static inline _u16 be16_to_cpu(_u16 x) {
	return _swap16(x);
}
static inline _u16 cpu_to_le16(_u16 x) {
	return x;
}
static inline _u16 le16_to_cpu(_u16 x) {
	return x;
}
static inline void cpu_to_be16(_u16 x, _u8* y1, _u8* y2) {
	_split16(x, y1, y2);
}
static inline _u16 be16_to_cpu(_u8 x1, _u8 x2) {
	return _combine16(x1, x2);
}
static inline void cpu_to_le16(_u16 x, _u8* y1, _u8* y2) {
	_split16(x, y2, y1);
}
static inline _u16 le16_to_cpu(_u8 x1, _u8 x2) {
	return _combine16(x2, x1);
}
static inline _u32 cpu_to_be32(_u32 x) {
	return _swap32(x);
}
static inline _u32 be32_to_cpu(_u32 x) {
	return _swap32(x);
}
static inline _u32 cpu_to_le32(_u32 x) {
	return x;
}
static inline _u32 le32_to_cpu(_u32 x) {
	return x;
}
static inline void cpu_to_be32(_u32 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4) {
	_split32(x, y1, y2, y3, y4);
}
static inline _u32 be32_to_cpu(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return _combine32(x1, x2, x3, x4);
}
static inline void cpu_to_le32(_u32 x, _u8* y1, _u8* y2, _u8* y3, _u8* y4) {
	_split32(x, y4, y3, y2, y1);
}
static inline _u32 le32_to_cpu(_u8 x1, _u8 x2, _u8 x3, _u8 x4) {
	return _combine32(x4, x3, x2, x1);
}

#elif defined ARCH_BE

#endif  // ARCH_LE or ARCH_BE

static inline _ucpu down_align(_ucpu base, _ucpu value) {
	return value & ~(base - 1);
}
static inline _ucpu up_align(_ucpu base, _ucpu value) {
	return (value + base - 1) & ~(base - 1);
}

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

#endif  // BTYPES_HPP
