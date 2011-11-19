/// @author  KATO Takeshi
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUPDATA_HH_
#define ARCH_X86_64_KERNEL_SETUPDATA_HH_

#include "basic_types.hh"


void setup_get_display_mode(u32* width, u32* height, u32* vram);
void setup_get_display_cursor(u32* row, u32* col);

struct setup_memory_dumpdata;
void setup_get_free_memdump(setup_memory_dumpdata** freedump, u32* num);
void setup_get_used_memdump(setup_memory_dumpdata** useddump, u32* num);

void setup_get_mp_info(u8** ptr);

const void* get_bootinfo(u32 tag_type);

namespace setup {

void get_log(char** buf, u32* cur, u32* size);

} // namespace setup


#endif  // include guard

