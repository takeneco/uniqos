// @file   arch/x86_64/kernel/idt.cc
// @author Kato Takeshi
// @brief  IDT ops.
//
// (C) 2010 Kato Takeshi.

#include "btypes.hh"
#include "native.hh"
#include "output.hh"
#include "string.hh"

namespace {

class idte
{
	typedef u64 type;
	type e[2];
public:
	void set(type offset, type seg, type ist, type dpl) {
		e[0] = (offset & 0x000000000000ffff)       |
		       (offset & 0x00000000ffff0000) << 32 |
		       (seg    & 0x000000000000ffff) << 16 |
		       (ist    & 0x0000000000000007) << 32 |
		       (dpl    & 0x0000000000000003) << 45 |
		       (         0x0000800000000e00); // Enable(P) and type flags.
		e[1] = (offset & 0xffffffff00000000) >> 32;
	}
	void disable() {
		e[0] = e[1] = 0;
	}
};

// アライメント？？？
idte idt[256];

}  // End of anonymous namespace

extern kern_output* kout;

void* IDTInit()
{
	kout->PutStr("&idt = ")->PutU64Hex((u64)idt)->PutC('\n');
	for (int i = 0; i < 1; i++) {
		idt[i].disable();
	}

	memory_fill(sizeof idt, 0, idt);

	arch::idt_ptr64 idtptr;
	idtptr.init(sizeof idt, idt);

	arch::native_lidt(&idtptr);

	return 0;
}
