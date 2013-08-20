/// @brief  メモリ幅指定のアクセス命令。

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
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

#ifndef ARCH_X86_64_INCLUDE_ARCH_MEM_OPS_HH_
#define ARCH_X86_64_INCLUDE_ARCH_MEM_OPS_HH_


namespace arch {

inline u8 mem_read8(const u8* ptr) {
	u8 r;
	asm volatile ("movb %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u16 mem_read16(const u16* ptr) {
	u16 r;
	asm volatile ("movw %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u32 mem_read32(const u32* ptr) {
	u32 r;
	asm volatile ("movl %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u64 mem_read64(const u64* ptr) {
	u64 r;
	asm volatile ("movq %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline void mem_write8(u8 x, u8* ptr) {
	asm volatile ("movb %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write16(u16 x, u16* ptr) {
	asm volatile ("movw %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write32(u32 x, u32* ptr) {
	asm volatile ("movl %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write64(u64 x, u64* ptr) {
	asm volatile ("movq %1, %0" : "=m" (*ptr) : "r" (x));
}

}  // namespace arch


#endif  // include guard

