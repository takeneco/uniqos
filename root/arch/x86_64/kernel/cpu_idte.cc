/// @file   cpu_idte.cc
/// @brief  interrupt entry.
//
// (C) 2010-2012 KATO Takeshi
//

#include "kerninit.hh"
#include "cpu_idte.hh"
#include "global_vars.hh"
#include <intr_ctl.hh>
#include "log.hh"
#include "native_ops.hh"
#include <cpu_node.hh>


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

	u64 rbx;
	u64 rbp;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;

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

	u64 rbx;
	u64 rbp;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;

	u64 error_code;
	u64 rip;
	u64 cs;
	u64 rf;
	u64 rsp;
	u64 ss;
};


void intr_update(idte* idt)
{
	//idte* idt = global_vars::gv.cpu_ctl_obj->get_idt();
	native::idt_ptr64 idtptr;
	idtptr.set(sizeof (idte) * 256, idt);

	native::lidt(&idtptr);
}

void unknown_exception(int n)
{
	log(1).str("Unknown exception 0x").u((u8)n, 16).str(" interrupted.").
		endl();

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

void intr_0x40();
void intr_0x41();
void intr_0x42();
void intr_0x43();
void intr_0x44();
void intr_0x45();
void intr_0x46();
void intr_0x47();
void intr_0x48();
void intr_0x49();
void intr_0x4a();
void intr_0x4b();
void intr_0x4c();
void intr_0x4d();
void intr_0x4e();
void intr_0x4f();

void intr_0x50();
void intr_0x51();
void intr_0x52();
void intr_0x53();
void intr_0x54();
void intr_0x55();
void intr_0x56();
void intr_0x57();
void intr_0x58();
void intr_0x59();
void intr_0x5a();
void intr_0x5b();
void intr_0x5c();
void intr_0x5d();
void intr_0x5e();
void intr_0x5f();

void intr_0x60();
void intr_0x61();
void intr_0x62();
void intr_0x63();
void intr_0x64();
void intr_0x65();
void intr_0x66();
void intr_0x67();
void intr_0x68();
void intr_0x69();
void intr_0x6a();
void intr_0x6b();
void intr_0x6c();
void intr_0x6d();
void intr_0x6e();
void intr_0x6f();

void intr_0x70();
void intr_0x71();
void intr_0x72();
void intr_0x73();
void intr_0x74();
void intr_0x75();
void intr_0x76();
void intr_0x77();
void intr_0x78();
void intr_0x79();
void intr_0x7a();
void intr_0x7b();
void intr_0x7c();
void intr_0x7d();
void intr_0x7e();
void intr_0x7f();

void intr_0x80();
void intr_0x81();
void intr_0x82();
void intr_0x83();
void intr_0x84();
void intr_0x85();
void intr_0x86();
void intr_0x87();
void intr_0x88();
void intr_0x89();
void intr_0x8a();
void intr_0x8b();
void intr_0x8c();
void intr_0x8d();
void intr_0x8e();
void intr_0x8f();

}

cause::type intr_init(idte* idt)
{
	struct idt_params {
		void (*handler)();
		uint seg;
		uint ist;
		uint dpl;
		u64 flags;
	} const exception_idt[] = {
		{ exception_intr_0x00, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x01, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x02, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x03, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x04, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x05, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x06, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x07, 1, 0, 0, idte::TRAP },
		//{ exception_intr_0x08, 1, 1, 0, idte::TRAP },
		{ exception_intr_0x08, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x09, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0a, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0b, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0c, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0d, 1, 2, 0, idte::TRAP },
		//{ exception_intr_0x0d, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0e, 1, 2, 0, idte::TRAP },
		//{ exception_intr_0x0e, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x0f, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x10, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x11, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x12, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x13, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x14, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x15, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x16, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x17, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x18, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x19, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1a, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1b, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1c, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1d, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1e, 1, 0, 0, idte::TRAP },
		{ exception_intr_0x1f, 1, 0, 0, idte::TRAP },
	};

	for (int i = 0; i <= 31; ++i) {
		const idt_params& e = exception_idt[i];
		idt[i].set(e.handler, 8 * e.seg, e.ist, e.dpl, e.flags);
	}

	void (* const idt_handler[])() = {
		intr_0x20, intr_0x21, intr_0x22, intr_0x23,
		intr_0x24, intr_0x25, intr_0x26, intr_0x27,
		intr_0x28, intr_0x29, intr_0x2a, intr_0x2b,
		intr_0x2c, intr_0x2d, intr_0x2e, intr_0x2f,

		intr_0x30, intr_0x31, intr_0x32, intr_0x33,
		intr_0x34, intr_0x35, intr_0x36, intr_0x37,
		intr_0x38, intr_0x39, intr_0x3a, intr_0x3b,
		intr_0x3c, intr_0x3d, intr_0x3e, intr_0x3f,

		intr_0x40, intr_0x41, intr_0x42, intr_0x43,
		intr_0x44, intr_0x45, intr_0x46, intr_0x47,
		intr_0x48, intr_0x49, intr_0x4a, intr_0x4b,
		intr_0x4c, intr_0x4d, intr_0x4e, intr_0x4f,

		intr_0x50, intr_0x51, intr_0x52, intr_0x53,
		intr_0x54, intr_0x55, intr_0x56, intr_0x57,
		intr_0x58, intr_0x59, intr_0x5a, intr_0x5b,
		intr_0x5c, intr_0x5d, intr_0x5e, intr_0x5f,

		intr_0x60, intr_0x61, intr_0x62, intr_0x63,
		intr_0x64, intr_0x65, intr_0x66, intr_0x67,
		intr_0x68, intr_0x69, intr_0x6a, intr_0x6b,
		intr_0x6c, intr_0x6d, intr_0x6e, intr_0x6f,

		intr_0x70, intr_0x71, intr_0x72, intr_0x73,
		intr_0x74, intr_0x75, intr_0x76, intr_0x77,
		intr_0x78, intr_0x79, intr_0x7a, intr_0x7b,
		intr_0x7c, intr_0x7d, intr_0x7e, intr_0x7f,

		intr_0x80, intr_0x81, intr_0x82, intr_0x83,
		intr_0x84, intr_0x85, intr_0x86, intr_0x87,
		intr_0x88, intr_0x89, intr_0x8a, intr_0x8b,
		intr_0x8c, intr_0x8d, intr_0x8e, intr_0x8f,
	};

	for (int i = 0; i < 0x90; ++i) {
		idt[0x20 + i].set(
		    idt_handler[i], 8 * 1, 1, 0, idte::INTR);
	}

	for (int i = 0x90+0x20; i < 256; i++) {
		idt[i].disable();
	}

	//intr_update(idt);
	return cause::OK;
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
	log(1)(__FILE__, __LINE__, __func__)();
	for (;;)
		native::hlt();
}

UNKNOWN_EXCEPTION(0x09)
UNKNOWN_EXCEPTION(0x0a)
UNKNOWN_EXCEPTION(0x0b)
UNKNOWN_EXCEPTION(0x0c)

extern "C" void on_exception_0x0d()
{
	reg_stack_witherr* stk = (reg_stack_witherr*)get_cpu_node()->
	    tss.get_ist(arch::cpu_ctl::IST_TRAP);
	stk -= 1;
	log(1)("stk:")(stk)();

	log(1)
	("rax:").u(stk->rax, 16)
	(", rbx:").u(stk->rbx, 16)
	(", rcx:").u(stk->rcx, 16)()
	("rdx:").u(stk->rdx, 16)
	(", rbp:").u(stk->rbp, 16)
	(", rsi:").u(stk->rsi, 16)()
	("rdi:").u(stk->rdi, 16)
	(", r8 :").u(stk->r8, 16)
	(", r9 :").u(stk->r9, 16)()
	("r10:").u(stk->r10, 16)
	(", r11:").u(stk->r11, 16)
	(", r12:").u(stk->r12, 16)()
	("r13:").u(stk->r13, 16)
	(", r14:").u(stk->r14, 16)
	(", r15:").u(stk->r15, 16)()
	("rip:").u(stk->rip, 16)
	(", cs :").u(stk->cs, 16)()
	("rsp:").u(stk->rsp, 16)
	(", ss :").u(stk->ss, 16)()
	("rf :").u(stk->rf, 16)
	(", err:").u(stk->error_code, 16)();

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

extern "C" void on_exception_0x0e()
{
	reg_stack_witherr* stk = (reg_stack_witherr*)get_cpu_node()->
	    tss.get_ist(arch::cpu_ctl::IST_TRAP);
	stk -= 1;
	log(1)("stk:")(stk)();

	log(1)
	("rax:").u(stk->rax, 16)
	(", rbx:").u(stk->rbx, 16)
	(", rcx:").u(stk->rcx, 16)()
	("rdx:").u(stk->rdx, 16)
	(", rbp:").u(stk->rbp, 16)
	(", rsi:").u(stk->rsi, 16)()
	("rdi:").u(stk->rdi, 16)
	(", r8 :").u(stk->r8, 16)
	(", r9 :").u(stk->r9, 16)()
	("r10:").u(stk->r10, 16)
	(", r11:").u(stk->r11, 16)
	(", r12:").u(stk->r12, 16)()
	("r13:").u(stk->r13, 16)
	(", r14:").u(stk->r14, 16)
	(", r15:").u(stk->r15, 16)()
	("rip:").u(stk->rip, 16)
	(", cs :").u(stk->cs, 16)()
	("rsp:").u(stk->rsp, 16)
	(", ss :").u(stk->ss, 16)()
	("rf :").u(stk->rf, 16)
	(", err:").u(stk->error_code, 16)();


	unknown_exception(0x0e);
}

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

