/// @file  arch/x86_64/include/arch/thread_ctl.hh
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_THREAD_CTL_HH_
#define ARCH_X86_64_INCLUDE_ARCH_THREAD_CTL_HH_


class thread;

namespace arch {

thread* get_current_thread();
void sleep_current_thread();

}  // namespace arch


#endif  // include guard

