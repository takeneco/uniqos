/**
 * @file    arch/x86_64/kernel/setup/mem.hpp
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   Memory ops.
 */
// (C) Kato.T 2010

#ifndef _ARCH_X86_64_KERNEL_SETUP_MEM_HPP
#define _ARCH_X86_64_KERNEL_SETUP_MEM_HPP

#include <cstddef>

#include "btypes.hpp"

/*
struct memmap_entry;
struct memmgr
{
	// 空き領域リスト
	memmap_entry* free_list;

	// 割り当て済み領域リスト
	memmap_entry* nofree_list;
};

void  memmgr_init(memmgr* mm);
void* memmgr_alloc(memmgr* mm, size_t size);
void  memmgr_free(memmgr* mm, void* p);
void* operator new(std::size_t s, memmgr*);
void* operator new[](std::size_t s, memmgr*);
void  operator delete(void* p, memmgr*);
void  operator delete[](void* p, memmgr*);
*/

/*
inline void set_video_term(video_term* p) {
	*reinterpret_cast<video_term**>(PH4_VIDEOTERM) = p;
}
inline video_term* get_video_term() {
	return *reinterpret_cast<video_term**>(PH4_VIDEOTERM);
}
*/

void* memory_move(void* dest, const void* src, std::size_t size);

struct acpi_memmap {
	enum type_value {
		MEMORY = 1,
		RESERVED = 2,
		ACPI = 3,
		NVS = 4,
		UNUSUABLE = 5
	};
	_u64 base;
	_u64 length;
	_u32 type;
	_u32 attr;
};


/*
// lzma wrapper

bool lzma_decode(
	memmgr*     mm,
	_u8*        src,
	std::size_t src_len,
	_u8*        dest,
	std::size_t dest_len);
*/

#endif  // Include guard.

