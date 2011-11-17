/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include "easy_alloc.hh"


void  mem_init();
void  mem_add(uptr adr, uptr bytes, bool avoid);
void* mem_alloc(uptr bytes, uptr align, bool forget);
void  mem_free(void* p);
bool  mem_reserve(uptr adr, uptr bytes, bool forget);
void  mem_alloc_info(easy_alloc_enum* x);
bool  mem_alloc_info_next(easy_alloc_enum* x, uptr* adr, uptr* bytes);

cause::stype pre_load(u32 magic, const u32* tag);
cause::stype post_load(u32* tag);


#endif  // include guard
