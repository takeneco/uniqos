// @file    arch/x86_64/kernel/setup/mem.hh
// @author  Kato Takeshi
// @brief   Memory ops.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_SETUP_MEM_HH_
#define _ARCH_X86_64_KERNEL_SETUP_MEM_HH_

#include <cstddef>

#include "btypes.hh"
#include "chain.hh"


struct memmap_entry
{
	u64 head;
	u64 bytes;  ///< If bytes == 0, not used.

	bichain_link<memmap_entry> _chain_link;

	void set(u64 _head, u64 _bytes) {
		head = _head;
		bytes = _bytes;
	}
	void unset() {
		bytes = 0;
	}
};

struct memmgr
{
	typedef bichain<memmap_entry, &memmap_entry::_chain_link>
	    memmap_entry_chain;

	/// 空き領域リスト
	memmap_entry_chain free_chain;
	//memmap_entry* free_list;

	/// 割り当て済み領域リスト
	memmap_entry_chain nofree_chain;
	//memmap_entry* nofree_list;
};

void  memmgr_init();
void* memmgr_alloc(std::size_t size, std::size_t align = 8);
void  memmgr_free(void* p);

struct setup_memmgr_dumpdata;
int   memmgr_dump(setup_memmgr_dumpdata* dumpto, int n);
/*
void* operator new(std::size_t s, memmgr*);
void* operator new[](std::size_t s, memmgr*);
void  operator delete(void* p, memmgr*);
void  operator delete[](void* p, memmgr*);
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


#endif  // Include guard.

