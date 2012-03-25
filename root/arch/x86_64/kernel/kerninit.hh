/// @file  kerninit.hh
//
// (C) 2010,2012 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_KERNINIT_HH_
#define ARCH_X86_64_KERNEL_KERNINIT_HH_

#include "basic_types.hh"


cause::stype cpupage_init();
cause::stype cpu_init();

typedef void (*intr_handler)();

void intr_init();

u64 convert_vadr_to_padr(const void* vadr);

// apic
namespace arch {
cause::stype apic_init();
void wait(u32 n);
}
void lapic_post_init_ipi();
void lapic_post_startup_ipi(u8 vec);

#endif  // Include guard.

