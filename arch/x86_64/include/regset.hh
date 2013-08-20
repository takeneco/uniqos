/// @file  regset.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_REGSET_HH_
#define ARCH_X86_64_INCLUDE_REGSET_HH_

#include <basic.hh>


namespace arch {

/// @brief  CPU register set.
struct regset
{
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rbx;
	u64 rbp;
	u64 rsi;
	u64 rdi;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;

	u64 rip;
	u16 cs;
	u16 ds;
	u16 es;
	u16 unused1_;
	u64 rf;
	u64 rsp;
	u16 ss;
	u16 fs;
	u16 gs;
	u16 unused2_;

	u64  cr3;

	regset(u64 text, u64 param, u64 stack, u64 stack_size) :
		rdi(param),
		rip(text),
		cs(0x08), ds(0x10), es(0x10),
		rsp(stack + stack_size),
		ss(0x10), fs(0x00), gs(0x00)
	{
		asm ("pushf; popq %0" : "=r"(rf));
	}
	//
	regset() {}
	//
};

}  // namespace arch


#endif  // include guard

