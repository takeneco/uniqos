/// @file bitops.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_BITOPS_HH_
#define CORE_INCLUDE_BITOPS_HH_

#include <native_bitops.hh>


/// @name ビット走査関数
/// find_first_setbit() は下位ビットから setbit を探す。
/// find_last_setbit() は上位ビットから setbit を探す。
/// @{

inline s16 find_first_setbit(u16 data) {
	return arch::find_first_setbit_16(data);
}
inline s16 find_first_setbit(u32 data) {
	return arch::find_first_setbit_32(data);
}
inline s16 find_first_setbit(u64 data) {
	return arch::find_first_setbit_64(data);
}
inline s16 find_last_setbit(u16 data)  {
	return arch::find_last_setbit_16(data);
}
inline s16 find_last_setbit(u32 data)  {
	return arch::find_last_setbit_32(data);
}
inline s16 find_last_setbit(u64 data)  {
	return arch::find_last_setbit_64(data);
}

/// @}


#endif  // CORE_INCLUDE_BITOPS_HH_

