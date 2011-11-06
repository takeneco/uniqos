/// @file  bootinfo.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_BOOTINFO_HH_
#define ARCH_X86_64_INCLUDE_BOOTINFO_HH_

#include "basic_types.hh"
#include "multiboot2.h"


namespace bootinfo {

enum { TYPE_MEMALLOC = 0x80000001 };
struct mem_alloc_entry {
	u64 adr;
	u64 bytes;
};
struct mem_alloc {
	u32 size;
	u32 type;
	mem_alloc_entry entries[0];
};

enum {
	// 物理メモリのこのアドレスに bootinfo が格納される。
	ADR = 0x500,
	MAX_BYTES = 0x80000 - ADR,
};

}  // namespace bootinfo


#endif  // include guard
