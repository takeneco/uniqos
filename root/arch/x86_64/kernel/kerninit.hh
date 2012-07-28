/// @file  kerninit.hh
//
// (C) 2010,2012 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_KERNINIT_HH_
#define ARCH_X86_64_KERNEL_KERNINIT_HH_

#include <basic.hh>


cause::type cpu_page_init();
cause::type mempool_init();
cause::type cpu_common_init();
cause::type cpu_setup();
cause::type intr_setup();
cause::type irq_setup();

namespace arch {

cause::type apic_init();
void wait(u32 n);

}  // namespace arch

void lapic_post_init_ipi();
void lapic_post_startup_ipi(u8 vec);


#endif  // include guard

