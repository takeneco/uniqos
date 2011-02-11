/// @file  task.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_TASK_HH_
#define ARCH_X86_64_KERNEL_TASK_HH_

#include "btypes.hh"


struct thread_state
{
	/// General Purpose Register
	/// RBX RCX RDX RBP RSI RDI R8-R15
	u64 gpr[14];
	u64 rsp;
	/// EFLAGSは32bitだが、pushfは64bit
	u64 eflags;
};


#endif  // Include guard.

