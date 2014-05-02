/// @file  kerninit.hh
//
// (C) 2010-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_SOURCE_KERNINIT_HH_
#define ARCH_X86_64_SOURCE_KERNINIT_HH_

#include <core/setup.hh>


cause::t cpu_page_init();
cause::t irq_setup();

namespace x86 {

cause::t thread_ctl_setup();
cause::t native_process_init();
cause::t cpu_ctl_setup();
cause::t cpu_setup();
cause::t create_boot_thread();

} // namespace x86

namespace arch {

cause::t apic_init();
void wait(u32 n);

}  // namespace arch

void lapic_post_init_ipi();
void lapic_post_startup_ipi(u8 vec);


#endif  // include guard

