/// @file   arch_specs.hh
/// @brief  x86_64 spec parameters.
///         used by architecture independent codes.
//
// (C) 2011-2013 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_SPECS_HH_
#define ARCH_X86_64_INCLUDE_ARCH_SPECS_HH_

#include <basic.hh>


namespace arch
{

enum {
	INTR_UPPER = 0x5f,
	INTR_COUNT = INTR_UPPER + 1,
};

}  // namespace arch


#endif  // Include guard.
