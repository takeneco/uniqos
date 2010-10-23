/// @file    memdump.hh
/// @brief   Free memory data structure.
//
// (C) 2010 Kato Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUP_MEMDUMP_HH_
#define ARCH_X86_64_KERNEL_SETUP_MEMDUMP_HH_

#include "btypes.hh"


struct setup_memmgr_dumpdata
{
	u64 head;
	u64 bytes;
};


#endif  // Include guard
