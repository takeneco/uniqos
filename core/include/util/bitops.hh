/// @file core/bitops.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#ifndef CORE_BITOPS_HH_
#define CORE_BITOPS_HH_

#include <arch/bitops.hh>


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


#endif  // CORE_BITOPS_HH_

