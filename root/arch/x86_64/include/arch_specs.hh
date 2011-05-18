/// @file   arch_specs.hh
/// @brief  x86_64 spec parameters.
///         used by architecture independent codes.
//
// (C) 2011 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_SPECS_HH_
#define ARCH_X86_64_INCLUDE_ARCH_SPECS_HH_

#include "basic_types.hh"


namespace arch
{

typedef u8 intr_vec;
enum {
	EXTERNAL_INTR_LOWER = 0x20,
	EXTERNAL_INTR_UPPER = 0x5f,
};

}  // namespace arch


#endif  // Include guard.
