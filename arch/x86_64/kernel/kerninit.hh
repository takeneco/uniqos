/// @file  kerninit.hh
//
// (C) 2010-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_KERNINIT_HH_
#define ARCH_X86_64_KERNEL_KERNINIT_HH_

#include <basic.hh>
#include <setup.hh>


cause::type cpu_page_init();
cause::type cpu_common_init();
cause::type irq_setup();

namespace x86 {
cause::t cpu_setup();
cause::t thread_ctl_setup();
cause::t create_boot_thread();
} // namespace x86

namespace arch {

cause::type apic_init();
void wait(u32 n);

}  // namespace arch

void lapic_post_init_ipi();
void lapic_post_startup_ipi(u8 vec);


#endif  // include guard

