// @file  misc.hh
//
// (C) 2010-2011 Kato Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUP_MISC_HH_
#define ARCH_X86_64_KERNEL_SETUP_MISC_HH_

#include "btypes.hh"


// memory alloc
void  memmgr_init();
void* memmgr_alloc(uptr size, uptr align = 8);
void  memmgr_free(void* p);

struct setup_memmgr_dumpdata;
int   memmgr_freemem_dump(setup_memmgr_dumpdata* dumpto, int n);
int   memmgr_nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n);

void* memory_move(void* dest, const void* src, uptr size);

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

// log
void setup_log();

// LZMA decode wrapper
u64 lzma_decode_size(const u8* src);
bool lzma_decode(
    u8*  src,
    uptr src_len,
    u8*  dest,
    uptr dest_len);


#endif  // Include guard

