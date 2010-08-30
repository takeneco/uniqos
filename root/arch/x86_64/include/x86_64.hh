/// @author Kato Takeshi
/// @brief  x86_64 hardware parameters.
///
/// (C) 2010 Kato Takeshi

#ifndef _ARCH_X86_64_INCLUDE_X86_64_HH_
#define _ARCH_X86_64_INCLUDE_X86_64_HH_

#include "btypes.hh"


enum {
	PAGE_SIZE = 4096,
	PAGE_SIZE_BITS = 12,

	IRQ_CPU_OFFSET = 0x00,
	IRQ_PIC_OFFSET = 0x20,
};


#endif  // Include guard.
