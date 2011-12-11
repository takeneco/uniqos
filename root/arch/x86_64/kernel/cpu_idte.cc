/// @file   cpu_idte.cc
/// @brief  interrupt entry.
//
// (C) 2010-2011 KATO Takeshi
//

#include "kerninit.hh"
#include "cpu_idte.hh"
#include "interrupt_control.hh"
#include "log.hh"
#include "native_ops.hh"


/// interrupt vector map
/// - 0x00-0x1f cpu reserved interrupts
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

void unknown_exception(int n)
{
	log().str("Unknown exception 0x").u(n, 16).str(" interrupted.").endl();

	for (;;)
		native::hlt();
}

}  // namespace

extern "C" {

void exception_intr_0x00();
void exception_intr_0x01();
void exception_intr_0x02();
void exception_intr_0x03();
void exception_intr_0x04();
void exception_intr_0x05();
void exception_intr_0x06();
void exception_intr_0x07();
void exception_intr_0x08();
void exception_intr_0x09();
void exception_intr_0x0a();
void exception_intr_0x0b();
void exception_intr_0x0c();
void exception_intr_0x0d();
void exception_intr_0x0e();
void exception_intr_0x0f();
void exception_intr_0x10();
void exception_intr_0x11();
void exception_intr_0x12();
void exception_intr_0x13();
void exception_intr_0x14();
void exception_intr_0x15();
void exception_intr_0x16();
void exception_intr_0x17();
void exception_intr_0x18();
void exception_intr_0x19();
void exception_intr_0x1a();
void exception_intr_0x1b();
void exception_intr_0x1c();
void exception_intr_0x1d();
void exception_intr_0x1e();
void exception_intr_0x1f();
void interrupt_20_handler();
void interrupt_0x5e_handler();
void interrupt_0x5f_handler();

void intr_0x20();
void intr_0x21();
void intr_0x22();
void intr_0x23();
void intr_0x24();
void intr_0x25();
void intr_0x26();
void intr_0x27();
void intr_0x28();
void intr_0x29();
void intr_0x2a();
void intr_0x2b();
void intr_0x2c();
void intr_0x2d();
void intr_0x2e();
void intr_0x2f();

void intr_0x30();
void intr_0x31();
void intr_0x32();
void intr_0x33();
void intr_0x34();
void intr_0x35();
void intr_0x36();
void intr_0x37();
void intr_0x38();
void intr_0x39();
void intr_0x3a();
void intr_0x3b();
void intr_0x3c();
void intr_0x3d();
void intr_0x3e();
void intr_0x3f();

}

void intr_init()
{
	struct idt_params {
		void (*handler)();
		uint seg;
		uint ist;
		uint dpl;
		u64 flags;
	} const exception_idt[] = {
		{ exception_intr_0x00, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x01, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x02, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x03, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x04, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x05, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x06, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x07, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x08, 1, 1, 0, arch::idte::TRAP },
		{ exception_intr_0x09, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x0a, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x0b, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x0c, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x0d, 1, 1, 0, arch::idte::TRAP },
		{ exception_intr_0x0e, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x0f, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x10, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x11, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x12, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x13, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x14, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x15, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x16, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x17, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x18, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x19, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1a, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1b, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1c, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1d, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1e, 1, 0, 0, arch::idte::TRAP },
		{ exception_intr_0x1f, 1, 0, 0, arch::idte::TRAP },
	};

	for (int i = 0; i <= 31; ++i) {
		const idt_params& e = exception_idt[i];
		idt_vec[i].set(e.handler, 8 * e.seg, e.ist, e.dpl, e.flags);
	}

	void (*idt_handler[])() = {
		intr_0x20,
		intr_0x21,
		intr_0x22,
		intr_0x23,
		intr_0x24,
		intr_0x25,
		intr_0x26,
		intr_0x27,
		intr_0x28,
		intr_0x29,
		intr_0x2a,
		intr_0x2b,
		intr_0x2c,
		intr_0x2d,
		intr_0x2e,
		intr_0x2f,
	};

	for (int i = 0; i < 16; ++i) {
		idt_vec[0x20 + i].set(
		    idt_handler[i], 8 * 1, 0, 0, arch::idte::INTR);
	}

	for (int i = 0x30; i < 256; i++) {
		idt_vec[i].disable();
	}

	idt_vec[0x20].set(
	    interrupt_20_handler, 8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x22].set(
	    interrupt_20_handler, 8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x23].set(
	    interrupt_20_handler, 8 * 1, 0, 0, arch::idte::INTR);
	idt_vec[0x30].set(
	    interrupt_20_handler, 8 * 1, 0, 0, arch::idte::INTR);

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

#define UNKNOWN_EXCEPTION(vec) \
	extern "C" void on_exception_##vec() { unknown_exception(vec); }

UNKNOWN_EXCEPTION(0x00)
UNKNOWN_EXCEPTION(0x01)
UNKNOWN_EXCEPTION(0x02)
UNKNOWN_EXCEPTION(0x03)
UNKNOWN_EXCEPTION(0x04)
UNKNOWN_EXCEPTION(0x05)
UNKNOWN_EXCEPTION(0x06)
UNKNOWN_EXCEPTION(0x07)

extern "C" void on_exception_0x08()
{
//	for (;;)
//		native::hlt();
	log()(__FILE__, __LINE__, __func__)();
}

UNKNOWN_EXCEPTION(0x09)
UNKNOWN_EXCEPTION(0x0a)
UNKNOWN_EXCEPTION(0x0b)
UNKNOWN_EXCEPTION(0x0c)

extern "C" void on_exception_0x0d()
{
	u64* stack=(u64*)0xffffffffffffc000;
	stack--;
/*
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

	unknown_exception(0x0d);
}

UNKNOWN_EXCEPTION(0x0e)
UNKNOWN_EXCEPTION(0x0f)
UNKNOWN_EXCEPTION(0x10)
UNKNOWN_EXCEPTION(0x11)
UNKNOWN_EXCEPTION(0x12)
UNKNOWN_EXCEPTION(0x13)
UNKNOWN_EXCEPTION(0x14)
UNKNOWN_EXCEPTION(0x15)
UNKNOWN_EXCEPTION(0x16)
UNKNOWN_EXCEPTION(0x17)
UNKNOWN_EXCEPTION(0x18)
UNKNOWN_EXCEPTION(0x19)
UNKNOWN_EXCEPTION(0x1a)
UNKNOWN_EXCEPTION(0x1b)
UNKNOWN_EXCEPTION(0x1c)
UNKNOWN_EXCEPTION(0x1d)
UNKNOWN_EXCEPTION(0x1e)
UNKNOWN_EXCEPTION(0x1f)

