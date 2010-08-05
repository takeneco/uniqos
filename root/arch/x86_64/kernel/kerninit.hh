// @file    arch/x86_64/kernel/kerninit.hh
// @author  Kato Takeshi
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_KERNINIT_HH_
#define _ARCH_X86_64_KERNEL_KERNINIT_HH_

#include "btypes.hh"

int cpu_init();

typedef void (*intr_handler)();

void intr_init();
void intr_set_handler(int intr, intr_handler hander);

u64 convert_vadr_to_padr(const void* vadr);

cause::stype phymemmgr_init();


#endif  // Include guard.

