/* FILE : include/types.hpp
 * VER  : 0.0.6
 * LAST : 2009-05-17
 * (C)  : 2008-2009 T.Kato
 * DESC : 共通で使う型・関数など
 */

#ifndef _INCLUDE_TYPES_H_
#define _INCLUDE_TYPES_H_

typedef unsigned char      __u8;
typedef   signed char      _s8;
typedef unsigned char      _u8;
typedef unsigned short     __u16;
typedef   signed short     _s16;
typedef unsigned short     _u16;
typedef unsigned long      __u32;
typedef   signed long      _s32;
typedef unsigned long      _u32;
typedef unsigned long long __u64;
typedef   signed long      _s64;
typedef unsigned long long _u64;

typedef _u32 _ucpu;
typedef _s32 _scpu;

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

/// for little endian
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

/// エラー番号
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

#endif // _INCLUDE_TYPES_H_
