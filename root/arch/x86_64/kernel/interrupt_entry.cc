/// @file   interrupt_control.cc
/// @brief  interrupt.
//
// (C) 2010-2011 KATO Takeshi
//

#include "kerninit.hh"
#include "chain.hh"
#include "idte.hh"
#include "interrupt_control.hh"
#include "log.hh"
#include "native_ops.hh"
#include "output.hh"
#include "string.hh"


/// interrupt vector map
/// - 0x00-0x1f cpu reserved
/// - 0x20-0x5f device interrupts
/// - 0x60-0xff unused

namespace {

struct reg_stack
{
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;

	u64 rip;
	u64 cs;
	u64 rf;
	u64 rsp;
	u64 ss;
};

struct reg_stack_witherr
{
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;

	u64 error_code;
	u64 rip;
	u64 cs;
	u64 rf;
	u64 rsp;
	u64 ss;
};

arch::idte idt_vec[256];

void intr_update()
{
	native::idt_ptr64 idtptr;
	idtptr.set(sizeof (arch::idte) * 256, &idt_vec[0]);

	native::lidt(&idtptr);
}

}  // namespace

extern "C" {

void exception_intr_0_handler();
void exception_intr_1_handler();
void exception_intr_2_handler();
void exception_intr_3_handler();
void exception_intr_4_handler();
void exception_intr_5_handler();
void exception_intr_6_handler();
void exception_intr_7_handler();
void exception_intr_8_handler();
void exception_intr_9_handler();
void exception_intr_10_handler();
void exception_intr_11_handler();
void exception_intr_12_handler();
void exception_intr_13_handler();
void exception_intr_14_handler();
void exception_intr_15_handler();
void exception_intr_16_handler();
void exception_intr_17_handler();
void exception_intr_18_handler();
void exception_intr_19_handler();
void interrupt_20_handler();
void interrupt_0x5e_handler();
void interrupt_0x5f_handler();

}

void intr_init()
{
	idt_vec[0].set(
	    reinterpret_cast<u64>(exception_intr_0_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[1].set(
	    reinterpret_cast<u64>(exception_intr_1_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[2].set(
	    reinterpret_cast<u64>(exception_intr_2_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[3].set(
	    reinterpret_cast<u64>(exception_intr_3_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[4].set(
	    reinterpret_cast<u64>(exception_intr_4_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[5].set(
	    reinterpret_cast<u64>(exception_intr_5_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[6].set(
	    reinterpret_cast<u64>(exception_intr_6_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[7].set(
	    reinterpret_cast<u64>(exception_intr_7_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[8].set(
	    reinterpret_cast<u64>(exception_intr_8_handler),
	    8 * 1, 1, 0, arch::idte::TRAP);
	idt_vec[9].set(
	    reinterpret_cast<u64>(exception_intr_9_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[10].set(
	    reinterpret_cast<u64>(exception_intr_10_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[11].set(
	    reinterpret_cast<u64>(exception_intr_11_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[12].set(
	    reinterpret_cast<u64>(exception_intr_12_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[13].set(
	    reinterpret_cast<u64>(exception_intr_13_handler),
	    8 * 1, 1, 0, arch::idte::TRAP);
	idt_vec[14].set(
	    reinterpret_cast<u64>(exception_intr_14_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[15].set(
	    reinterpret_cast<u64>(exception_intr_15_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[16].set(
	    reinterpret_cast<u64>(exception_intr_16_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[17].set(
	    reinterpret_cast<u64>(exception_intr_17_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[18].set(
	    reinterpret_cast<u64>(exception_intr_18_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);
	idt_vec[19].set(
	    reinterpret_cast<u64>(exception_intr_19_handler),
	    8 * 1, 0, 0, arch::idte::TRAP);

	for (int i = 20; i < 256; i++) {
		idt_vec[i].disable();
	}

	idt_vec[0x20].set(
	    reinterpret_cast<u64>(interrupt_20_handler),
	    8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x22].set(
	    reinterpret_cast<u64>(interrupt_20_handler),
	    8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x30].set(
	    reinterpret_cast<u64>(interrupt_20_handler),
	    8 * 1, 0, 0, arch::idte::INTR);

	idt_vec[0x5e].set(
	    interrupt_0x5e_handler, 8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x5f].set(
	    interrupt_0x5f_handler, 8 * 1, 0, 0, arch::idte::INTR);

	intr_update();
}

void intr_set_handler(int intr, intr_handler handler)
{
	idt_vec[intr].set(
	    reinterpret_cast<u64>(handler),
	    8 * 1, 0, 0, arch::idte::INTR);

	intr_update();
}

extern "C" void on_exception_intr_0()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 0");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_1()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 1");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_2()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 2");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_3()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 3");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_4()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 4");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_5()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 5");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_6()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 6");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_7()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 7");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_8()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 8");
	}

	kout->PutU64Hex((u64)(&kout));

//	for (;;)
//		native::hlt();
}

extern "C" void on_exception_intr_9()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 9");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_10()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 10");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_11()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 11");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_12()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 12");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_13()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 13");
	}
/*
	log()("&kout=")(kout)();
	u64* stack=(u64*)0xffffffffffffc000;
	stack--;
	log()("ss =").u(*stack--, 16)();
	log()("rsp=").u(*stack--, 16)();
	log()("rf =").u(*stack--, 16)();
	log()("cs =").u(*stack--, 16)();
	log()("rip=").u(*stack--, 16)();
	log()("err=").u(*stack--, 16)();
	log()("r11=").u(*stack--, 16)();
	log()("r10=").u(*stack--, 16)();
	log()("r9 =").u(*stack--, 16)();
	log()("r8 =").u(*stack--, 16)();
	log()("rdi=").u(*stack--, 16)();
	log()("rsi=").u(*stack--, 16)();
	log()("rdx=").u(*stack--, 16)();
	log()("rcx=").u(*stack--, 16)();
	log()("rax=").u(*stack--, 16)();
*/
	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_14()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 14");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_15()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 15");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_16()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 16");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_17()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 17");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_18()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 18");
	}

	for (;;)
		native::hlt();
}

extern "C" void on_exception_intr_19()
{
	kern_output* kout = kern_get_out();
	if (kout) {
		kout->PutStr("exception 19");
	}

	for (;;)
		native::hlt();
}


