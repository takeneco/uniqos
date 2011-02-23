/// @author  KATO Takeshi
//
// (C) 2010 KATO Takeshi

#ifndef ARCH_X86_64_KERNEL_SETUPDATA_HH_
#define ARCH_X86_64_KERNEL_SETUPDATA_HH_

#include "btypes.hh"


void setup_get_display_mode(u32* width, u32* height, u32* vram);
void setup_get_display_cursor(u32* row, u32* col);

struct setup_memmgr_dumpdata;
void setup_get_free_memdump(setup_memmgr_dumpdata** freedump, u32* num);
void setup_get_used_memdump(setup_memmgr_dumpdata** useddump, u32* num);

void setup_get_mp_info(u8** ptr);


#endif  // Include guard.

