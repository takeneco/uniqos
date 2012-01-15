/// @file  regset.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_REGSET_HH_
#define ARCH_X86_64_KERNEL_REGSET_HH_

#include "basic_types.hh"


struct regset
{
	u64 rax;
	u64 rbx;
	u64 rcx;
	u64 rdx;
	u64 rbp;
	u64 rsi;
	u64 rdi;
	u64 rsp;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
	u64 rip;
	u64 eflags;
	u16 cs;
	u16 ds;
	u16 es;
	u16 fs;
	u16 gs;
	u16 ss;
};


#endif  // include guard

