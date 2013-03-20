/// @file   arch/x86_64/kernel/string.cc
/// @brief  Memory ops for x86_64.
//
/// clang ではメモリ操作が memcpy, memset, memcmp などに置換されてしまうため、
/// memcpy, memset, memcmp を独自実装するときはアセンブラで書くことにする。

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <arch.hh>
#include <string.hh>


#ifdef ARCH_PROVIDES_MEM_FILL

void mem_fill(uptr bytes, u8 c, void* dest)
{
	asm volatile ("rep stosb" : : "c"(bytes), "a"(c), "D"(dest));
}

#endif  // ARCH_PROVIDES_MEM_FILL

