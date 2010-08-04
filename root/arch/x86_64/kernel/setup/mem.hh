// @file    arch/x86_64/kernel/setup/mem.hh
// @author  Kato Takeshi
// @brief   Memory ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_SETUP_MEM_HH_
#define _ARCH_X86_64_KERNEL_SETUP_MEM_HH_

#include <cstddef>

#include "btypes.hh"


struct memmap_entry
{
	memmap_entry* prev;
	memmap_entry* next;
	u64 head;
	u64 bytes;  ///< If bytes == 0, not used.
};

struct memmgr
{
	/// 空き領域リスト
	memmap_entry* free_list;

	/// 割り当て済み領域リスト
	memmap_entry* nofree_list;
};

void  memmgr_init(memmgr* mm);
void* memmgr_alloc(memmgr* mm, std::size_t size, std::size_t align = 8);
void  memmgr_free(memmgr* mm, void* p);

struct setup_memmgr_dumpdata;
int   memmgr_dump(const memmgr* mm, setup_memmgr_dumpdata* dumpto, int n);

void* operator new(std::size_t s, memmgr*);
void* operator new[](std::size_t s, memmgr*);
void  operator delete(void* p, memmgr*);
void  operator delete[](void* p, memmgr*);

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


#endif  // Include guard.

