// @file    arch/x86_64/kernel/setupdata.hh
// @author  Kato Takeshi
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_SETUPDATA_HH_
#define _ARCH_X86_64_KERNEL_SETUPDATA_HH_

#include "btypes.hh"


void setup_get_display_mode(u32* width, u32* height, u32* vram);
void setup_get_display_cursor(u32* row, u32* col);


#endif  // Include guard.

