// @file    arch/x86_64/kernel/setupdata.hh
// @author  Kato Takeshi
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_SETUPDATA_HH_
#define _ARCH_X86_64_KERNEL_SETUPDATA_HH_

#include "btypes.hh"


void SetupGetCurrentDisplayMode(u32* Width, u32* Height, u32* VRam);
void SetupGetCurrentDisplayCursor(u32* Row, u32* Col);


#endif  // Include guard.

