/// @author  KATO Takeshi
/// @brief   Memory ops.
//
// (C) 2010 KATO Takeshi

#ifndef ARCH_X86_64_KERNEL_SETUP_MEM_HH_
#define ARCH_X86_64_KERNEL_SETUP_MEM_HH_

#include <cstddef>

#include "btypes.hh"
#include "chain.hh"


struct memmap_entry
{
	u64 head;
	u64 bytes;  ///< If bytes == 0, not used.

	bichain_link<memmap_entry> _chain_link;

	void set(u64 head_, u64 bytes_) {
		head = head_;
		bytes = bytes_;
	}
	void unset() {
		bytes = 0;
	}
};

void  memmgr_init();
void* memmgr_alloc(std::size_t size, std::size_t align = 8);
void  memmgr_free(void* p);

struct setup_memmgr_dumpdata;
int   memmgr_freemem_dump(setup_memmgr_dumpdata* dumpto, int n);
int   memmgr_nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n);

void* memory_move(void* dest, const void* src, std::size_t size);

struct acpi_memmap {
	enum type_value {
		MEMORY = 1,
		RESERVED = 2,
		ACPI = 3,
		NVS = 4,
		UNUSUABLE = 5
	};
	u64 base;
	u64 length;
	u32 type;
	u32 attr;
};


#endif  // Include guard.

