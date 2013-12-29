/// @file  flags.hh
/// @brief x86_64 hardware flags.
//
// (C) 2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_FLAGS_HH_
#define ARCH_X86_64_INCLUDE_FLAGS_HH_

#include <basic.hh>


namespace x86 {

namespace REGFLAGS {

enum MASK {
	CF   = 1 << 0,   // Carry Flag
	PF   = 1 << 2,   // Parity Flag
	AF   = 1 << 4,   // Auxiliary Carry Flag
	ZF   = 1 << 6,   // Zero Flag
	SF   = 1 << 7,   // Sign Flag
	TF   = 1 << 8,   // Trap Flag
	IF   = 1 << 9,   // Interrupt Enable Flag
	DF   = 1 << 10,  // Direction Flag
	OF   = 1 << 11,  // Overflow Flag
	IOPL = 3 << 12,  // I/O Privilege Level
	NT   = 1 << 14,  // Nested Task
	RF   = 1 << 16,  // Resume Flag
	VM   = 1 << 17,  // Virtual-8086 Mode
	AC   = 1 << 18,  // Alignment Check
	VIF  = 1 << 19,  // Virtual Interrupt Flag
	VIP  = 1 << 20,  // Virtual Interrupt Pending
	ID   = 1 << 21,  // ID Flag
};

}  // REGFLAGS

}  // namespace x86

#endif  // include guard

