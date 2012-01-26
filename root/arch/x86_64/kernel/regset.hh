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
};

class thread
{
public:
	thread(u64 _rip, u64 _rsp) {
		rs.rip = _rip;
		rs.rsp = _rsp;
		rs.cs = 8;
		rs.ds = rs.es = rs.fs = rs.gs = rs.ss = 16;
	}

public:
	regset rs;
};


#endif  // include guard

