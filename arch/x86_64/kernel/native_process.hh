/// @file  native_process.hh
//
// (C) 2014 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_NATIVE_PROCESS_HH_
#define ARCH_X86_64_KERNEL_NATIVE_PROCESS_HH_

#include <process.hh>


namespace x86 {

class native_process : public process
{
public:
	native_process();
};

}  // namespace x86


#endif  // include guard
