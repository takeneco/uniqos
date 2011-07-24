/// @file  kerninit.hh
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_KERNINIT_HH_
#define ARCH_X86_64_KERNEL_KERNINIT_HH_

#include "basic_types.hh"


int cpu_init();

typedef void (*intr_handler)();

void intr_init();
void intr_set_handler(int intr, intr_handler hander);

u64 convert_vadr_to_padr(const void* vadr);

// apic
namespace arch {
cause::stype apic_init();
void wait(u32 n);
}

#endif  // Include guard.

