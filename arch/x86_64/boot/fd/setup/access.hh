// @file    arch/x86_64/kernel/setup/access.hh
// @brief   Setup data access methods.
//
// (C) 2010 Kato Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUP_ACCESS_HH_
#define ARCH_X86_64_KERNEL_SETUP_ACCESS_HH_

#include "basic_types.hh"
#include "setup.h"


template<class T> inline T setup_get_value(u64 off) {
	return *reinterpret_cast<T*>((SETUP_DATA_SEG << 4) + off);
}
template<class T> inline void setup_set_value(u64 off, T val) {
	*reinterpret_cast<T*>((SETUP_DATA_SEG << 4) + off) = val;
}
template<class T> inline T* setup_get_ptr(u64 off) {
	return reinterpret_cast<T*>((SETUP_DATA_SEG << 4) + off);
}


#endif  // include guard

