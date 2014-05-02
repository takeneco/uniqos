/// @file  arch/x86_64/include/arch/global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_GLOBAL_VARS_HH_
#define ARCH_X86_64_INCLUDE_ARCH_GLOBAL_VARS_HH_


class irq_ctl;

namespace x86 {

class native_cpu_ctl;
class thread_ctl;

}  // namespace x86


namespace global_vars {

struct _arch
{
	x86::native_cpu_ctl* native_cpu_ctl_obj;

	irq_ctl*             irq_ctl_obj;

	x86::thread_ctl*     thread_ctl_obj;

	void*                bootinfo;
};

extern _arch arch;

}  // namespace global_vars


#endif  // include guard

