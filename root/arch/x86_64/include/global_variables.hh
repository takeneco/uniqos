/// @file  global_variables.hh
/// @brief Global variables declaration.
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_
#define ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_


class physical_memory;
class kernel_memory;


namespace global_variable {


struct global_variable_ {
	physical_memory* pmem_ctrl;
	kernel_memory*   kmem_ctrl;
};


extern global_variable_ gv;

}  // namespace global_variable


#endif  // Include guard
