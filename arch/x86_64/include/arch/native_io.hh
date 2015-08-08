/// @file native_io.hh
/// @brief  C++ から呼び出すアセンブラ命令
//
// (C) 2015 KATO Takeshi
//

#ifndef ARCH_NATIVE_IO_HH_
#define ARCH_NATIVE_IO_HH_

#include <core/basic.hh>


namespace arch {

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

}  // namespace native


#endif  // ARCH_NATIVE_IO_HH_

