/// @file  bootinfo.hh
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_BOOTINFO_HH_
#define ARCH_X86_64_INCLUDE_BOOTINFO_HH_

#include <basic_types.hh>
#include <multiboot2.h>


namespace bootinfo {

enum {
	TYPE_MEMALLOC = 0x80000001,
	TYPE_LOG,
	TYPE_BUNDLE,
};

struct mem_alloc_entry {
	u64 adr;
	u64 bytes;
};
struct mem_alloc {
	u32 type;
	u32 size;
	mem_alloc_entry entries[0];
};

struct log {
	u32 type;
	u32 size;
	u8 log[0];
};

enum {
	MAX_BYTES = 0x60000,

	// Available memory end during kernel boot.
	BOOTHEAP_END   = 0x01ffffff,
};

const void* get_bootinfo(u32 tag_type);

}  // namespace bootinfo


#endif  // include guard

