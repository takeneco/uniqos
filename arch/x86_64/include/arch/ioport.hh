/// @file   arch/ioport.hh
/// @brief  I/O port instructions.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
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

#ifndef ARCH_IOPORT_HH_
#define ARCH_IOPORT_HH_

#include <core/basic.hh>


namespace arch {

inline void ioport_out8(u8 data, u16 port) {
	asm volatile ("outb %0,%1" : : "a" (data), "dN" (port));
}
inline void ioport_out16(u16 data, u16 port) {
	asm volatile ("outw %0,%1" : : "a" (data), "dN" (port));
}
inline void ioport_out32(u32 data, u16 port) {
	asm volatile ("outl %0,%1" : : "a" (data), "dN" (port));
}
inline u8 ioport_in8(u16 port) {
	u8 data;
	asm volatile ("inb %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline u16 ioport_in16(u16 port) {
	u16 data;
	asm volatile ("inw %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline u32 ioport_in32(u16 port) {
	u32 data;
	asm volatile ("inl %1,%0" : "=a" (data) : "dN" (port));
	return data;
}

}  // namespace arch


#endif  // ARCH_IOPORT_HH_

