/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include "easy_alloc.hh"


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

void  mem_init();
void  mem_add(uptr adr, uptr bytes, bool avoid);
void* mem_alloc(uptr bytes, uptr align, bool forget);
void  mem_free(void* p);
bool  mem_reserve(uptr adr, uptr bytes, bool forget);
void  mem_alloc_info(easy_alloc_enum* x);
bool  mem_alloc_info_next(easy_alloc_enum* x, uptr* adr, uptr* bytes);

cause::stype pre_load(u32 magic, const u32* tag);
cause::stype post_load(u32* tag);

class text_vga;
extern text_vga vga_dev;

class log_file;
extern log_file vga_log;


#endif  // include guard
