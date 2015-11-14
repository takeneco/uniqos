/// @file   cpu_idte.cc
/// @brief  interrupt entry.
//
// (C) 2010-2014 KATO Takeshi
//

#include "kerninit.hh"
#include "cpu_idte.hh"
#include <core/intr_ctl.hh>
#include <core/log.hh>
#include <native_cpu_node.hh>
#include <native_ops.hh>


/// interrupt vector map
/// - 0x00-0x1f cpu reserved interrupts
/// - 0x20-0x5f device interrupts
/// - 0x60-0xff unused

extern "C" {

void except_0x00();
void except_0x01();
void except_0x02();
void except_0x03();
void except_0x04();
void except_0x05();
void except_0x06();
void except_0x07();
void except_0x08();
void except_0x09();
void except_0x0a();
void except_0x0b();
void except_0x0c();
void except_0x0d();
void except_0x0e();
void except_0x0f();
void except_0x10();
void except_0x11();
void except_0x12();
void except_0x13();
void except_0x14();
void except_0x15();
void except_0x16();
void except_0x17();
void except_0x18();
void except_0x19();
void except_0x1a();
void except_0x1b();
void except_0x1c();
void except_0x1d();
void except_0x1e();
void except_0x1f();

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

}  // extern "C"

cause::t intr_init(idte* idt)
{
	struct idt_params {
		void (*handler)();
		uint seg;
		uint ist;
		uint dpl;
		u64 flags;
	} static const exception_idt[] = {
		{ except_0x00, 1, 1, 0, idte::TRAP },
		{ except_0x01, 1, 1, 0, idte::TRAP },
		{ except_0x02, 1, 1, 0, idte::TRAP },
		{ except_0x03, 1, 1, 0, idte::TRAP },
		{ except_0x04, 1, 1, 0, idte::TRAP },
		{ except_0x05, 1, 1, 0, idte::TRAP },
		{ except_0x06, 1, 1, 0, idte::TRAP },
		{ except_0x07, 1, 1, 0, idte::TRAP },
		{ except_0x08, 1, 1, 0, idte::TRAP },
		{ except_0x09, 1, 1, 0, idte::TRAP },
		{ except_0x0a, 1, 1, 0, idte::TRAP },
		{ except_0x0b, 1, 1, 0, idte::TRAP },
		{ except_0x0c, 1, 1, 0, idte::TRAP },
		{ except_0x0d, 1, 1, 0, idte::TRAP },
		{ except_0x0e, 1, 1, 0, idte::TRAP },
		{ except_0x0f, 1, 1, 0, idte::TRAP },
		{ except_0x10, 1, 1, 0, idte::TRAP },
		{ except_0x11, 1, 1, 0, idte::TRAP },
		{ except_0x12, 1, 1, 0, idte::TRAP },
		{ except_0x13, 1, 1, 0, idte::TRAP },
		{ except_0x14, 1, 1, 0, idte::TRAP },
		{ except_0x15, 1, 1, 0, idte::TRAP },
		{ except_0x16, 1, 1, 0, idte::TRAP },
		{ except_0x17, 1, 1, 0, idte::TRAP },
		{ except_0x18, 1, 1, 0, idte::TRAP },
		{ except_0x19, 1, 1, 0, idte::TRAP },
		{ except_0x1a, 1, 1, 0, idte::TRAP },
		{ except_0x1b, 1, 1, 0, idte::TRAP },
		{ except_0x1c, 1, 1, 0, idte::TRAP },
		{ except_0x1d, 1, 1, 0, idte::TRAP },
		{ except_0x1e, 1, 1, 0, idte::TRAP },
		{ except_0x1f, 1, 1, 0, idte::TRAP },
	};

	for (int i = 0; i <= 31; ++i) {
		const idt_params& e = exception_idt[i];
		idt[i].set(e.handler, 8 * e.seg, e.ist, e.dpl, e.flags);
	}

	static void (* const idt_handler[])() = {
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

	return cause::OK;
}

void except_dump(int vec, const char* msg)
{
	x86::native_cpu_node* cn = x86::get_native_cpu_node();
	arch::regset* rs = cn->intr_buf.running_thread_regset;

	log(1)(msg)(" v=").u(vec)()
	    ("rax:").x(rs->rax, 16)
	    (", rcx:").x(rs->rcx, 16)
	    (", rdx:").x(rs->rdx, 16)()
	    ("rbx:").x(rs->rbx, 16)
	    (", rbp:").x(rs->rbp, 16)
	    (", rsi:").x(rs->rsi, 16)()
	    ("rdi:").x(rs->rdi, 16)
	    (", r8 :").x(rs->r8, 16)
	    (", r9 :").x(rs->r9, 16)()
	    ("r10:").x(rs->r10, 16)
	    (", r11:").x(rs->r11, 16)
	    (", r12:").x(rs->r12, 16)()
	    ("r13:").x(rs->r13, 16)
	    (", r14:").x(rs->r14, 16)
	    (", r15:").x(rs->r15, 16)()
	    ("cs:").x(rs->cs, 4)
	    (", rip:").x(rs->rip, 16)()
	    ("ss:").x(rs->ss, 4)
	    (", rsp:").x(rs->rsp, 16)()
	    ("ds:").x(rs->ds, 4)
	    (", es:").x(rs->es, 4)
	    (", fs:").x(rs->fs, 4)
	    (", gs:").x(rs->gs, 4)
	    (", rf:").x(rs->rf, 8)()
	    ("cr3:").x(rs->cr3, 16)();

	for (;;)
		native::hlt();
}

const char* UNKNOWN_INTR = "!!!UNKNOWN INTERRUPT";

extern "C" void on_except_0x00() {
	except_dump(0x00, "Divide Error Exception (#DE)");
}
extern "C" void on_except_0x01() {
	except_dump(0x01, "Debug Exception (#DB)");
}
extern "C" void on_except_0x02() {
	except_dump(0x02, "NMI Interrupt");
}
extern "C" void on_except_0x03() {
	except_dump(0x03, "Breakpoint Exception (#BP)");
}
extern "C" void on_except_0x04() {
	except_dump(0x04, "Overflow Exception (#OF)");
}
extern "C" void on_except_0x05() {
	except_dump(0x05, "BOUND Range Exceeded Exception (#BR)");
}
extern "C" void on_except_0x06() {
	except_dump(0x06, "Invalid Opcode Exception (#UD)");
}
extern "C" void on_except_0x07() {
	except_dump(0x07, "Device Not Available Exception (#NM)");
}
extern "C" void on_except_0x08() {
	except_dump(0x08, "Double Fault Exception (#TS)");
}
extern "C" void on_except_0x09() {
	except_dump(0x09, "Coprocessor Segment Overrun");
}
extern "C" void on_except_0x0a() {
	except_dump(0x0a, "Invalid TSS Exception (#TS)");
}
extern "C" void on_except_0x0b() {
	except_dump(0x0b, "Segment Not Present (#NP)");
}
extern "C" void on_except_0x0c() {
	except_dump(0x0c, "Stack Fault Exception (#SS)");
}
extern "C" void on_except_0x0d() {
	except_dump(0x0d, "General Protection Exception (#GP)");
}
extern "C" void on_except_0x0e() {
	except_dump(0x0e, "Page-Fault Exception (#PF)");
}
extern "C" void on_except_0x0f() {
	except_dump(0x0f, UNKNOWN_INTR);
}
extern "C" void on_except_0x10() {
	except_dump(0x10, "x87 FPU Floating-Point Error (#MF)");
}
extern "C" void on_except_0x11() {
	except_dump(0x11, "Alignment Check Exception (#AC)");
}
extern "C" void on_except_0x12() {
	except_dump(0x12, "Machine-Check Exception (#MC)");
}
extern "C" void on_except_0x13() {
	except_dump(0x13, "SIMD Floating-Point Exception (#XM)");
}
extern "C" void on_except_0x14() {
	except_dump(0x14, UNKNOWN_INTR);
}
extern "C" void on_except_0x15() {
	except_dump(0x15, UNKNOWN_INTR);
}
extern "C" void on_except_0x16() {
	except_dump(0x16, UNKNOWN_INTR);
}
extern "C" void on_except_0x17() {
	except_dump(0x17, UNKNOWN_INTR);
}
extern "C" void on_except_0x18() {
	except_dump(0x18, UNKNOWN_INTR);
}
extern "C" void on_except_0x19() {
	except_dump(0x19, UNKNOWN_INTR);
}
extern "C" void on_except_0x1a() {
	except_dump(0x1a, UNKNOWN_INTR);
}
extern "C" void on_except_0x1b() {
	except_dump(0x1b, UNKNOWN_INTR);
}
extern "C" void on_except_0x1c() {
	except_dump(0x1c, UNKNOWN_INTR);
}
extern "C" void on_except_0x1d() {
	except_dump(0x1d, UNKNOWN_INTR);
}
extern "C" void on_except_0x1e() {
	except_dump(0x1e, UNKNOWN_INTR);
}
extern "C" void on_except_0x1f() {
	except_dump(0x1f, UNKNOWN_INTR);
}

