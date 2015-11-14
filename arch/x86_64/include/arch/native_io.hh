/// @file native_io.hh
/// @brief  C++ から呼び出すアセンブラ命令

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

#ifndef ARCH_NATIVE_IO_HH_
#define ARCH_NATIVE_IO_HH_

#include <core/basic.hh>


namespace arch {

/// Read memory by native operation.
/// @{

inline u8 read_u8(const void* mem)
{
	u8 r;
	asm volatile ("movb %1, %0" :
	              "=r" (r) :
	              "m" (*static_cast<const u8*>(mem)));
	return r;
}

inline u16 read_u16(const void* mem)
{
	u16 r;
	asm volatile ("movw %1, %0" :
	              "=r" (r) :
	              "m" (*static_cast<const u16*>(mem)));
	return r;
}

inline u32 read_u32(const void* mem)
{
	u32 r;
	asm volatile ("movl %1, %0" :
	              "=r" (r) :
	              "m" (*static_cast<const u32*>(mem)));
	return r;
}

inline u64 read_u64(const void* mem)
{
	u64 r;
	asm volatile ("movq %1, %0" :
	              "=r" (r) :
	              "m" (*static_cast<const u64*>(mem)));
	return r;
}

/// @}

/// Write memory by native operation.
/// @{

inline void write_u8(u8 data, void* mem)
{
	asm volatile ("movb %1, %0" :
	              "=m" (*static_cast<u8*>(mem)) : "r" (data));
}

inline void write_u16(u16 data, void* mem)
{
	asm volatile ("movw %1, %0" :
	              "=m" (*static_cast<u16*>(mem)) : "r" (data));
}

inline void write_u32(u32 data, void* mem)
{
	asm volatile ("movl %1, %0" :
	              "=m" (*static_cast<u32*>(mem)) : "r" (data));
}

inline void write_u64(u64 data, void* mem)
{
	asm volatile ("movq %1, %0" :
	              "=m" (*static_cast<u64*>(mem)) : "r" (data));
}

/// @}

/// IO Output.
/// @{

inline void ioport_out8(u8 data, u16 port)
{
	asm volatile ("outb %0,%1" : : "a" (data), "dN" (port));
}

inline void ioport_out16(u16 data, u16 port)
{
	asm volatile ("outw %0,%1" : : "a" (data), "dN" (port));
}

inline void ioport_out32(u32 data, u16 port)
{
	asm volatile ("outl %0,%1" : : "a" (data), "dN" (port));
}

/// @}

/// IO Input
/// @{

inline u8 ioport_in8(u16 port)
{
	u8 data;
	asm volatile ("inb %1,%0" : "=a" (data) : "dN" (port));
	return data;
}

inline u16 ioport_in16(u16 port)
{
	u16 data;
	asm volatile ("inw %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline u32 ioport_in32(u16 port)
{
	u32 data;
	asm volatile ("inl %1,%0" : "=a" (data) : "dN" (port));
	return data;
}

/// @}

}  // namespace arch


#endif  // ARCH_NATIVE_IO_HH_

