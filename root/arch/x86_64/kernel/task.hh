/// @file  task.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_TASK_HH_
#define ARCH_X86_64_KERNEL_TASK_HH_

#include "btypes.hh"


// Task State Segment
struct TSS64
{
	u32 reserved_1;
	u32 rsp0l;
	u32 rsp0h;
	u32 rsp1l;
	u32 rsp1h;
	u32 rsp2l;
	u32 rsp2h;
	u32 reserved_2;
	u32 reserved_3;
	u32 ist1l;
	u32 ist1h;
	u32 ist2l;
	u32 ist2h;
	u32 ist3l;
	u32 ist3h;
	u32 ist4l;
	u32 ist4h;
	u32 ist5l;
	u32 ist5h;
	u32 ist6l;
	u32 ist6h;
	u32 ist7l;
	u32 ist7h;
	u32 reserved_4;
	u32 reserved_5;
	u16 reserved_6;
	u16 iomap_base;
};


#endif  // Include guard.

