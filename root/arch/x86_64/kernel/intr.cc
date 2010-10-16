/// @author KATO Takeshi
/// @brief  IDT ops.
//
// (C) 2010 KATO Takeshi

#include "kerninit.hh"

#include "native.hh"
#include "output.hh"
#include "string.hh"


namespace {

class idte
{
	typedef u64 type;
	type e[2];
public:
	enum FLAGS {
		INTR = 0x0e0000000000,
		TRAP = 0x0f0000000000,
	};
	void set(type offset, type seg, type ist, type dpl, type flags) {
		e[0] = (offset & 0x000000000000ffff)       |
		       (offset & 0x00000000ffff0000) << 32 |
		       (seg    & 0x000000000000ffff) << 16 |
		       (ist    & 0x0000000000000007) << 32 |
		       (dpl    & 0x0000000000000003) << 45 |
		       (flags  & 0x00001f0000000000)       |
		       (         0x0000800000000000); // Enable(P) flag.
		e[1] = (offset & 0xffffffff00000000) >> 32;
	}
	void disable() {
		e[0] = e[1] = 0;
	}

	u64 get(int i) const { return e[i]; }
};

idte idt_vec[256];

void intr_update()
{
	idt_ptr64 idtptr;
	idtptr.init(sizeof (idte) * 256, &idt_vec[0]);

	native_lidt(&idtptr);
}

}  // End of anonymous namespace

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

}

void intr_init()
{
	idt_vec[0].set(
	    reinterpret_cast<u64>(exception_intr_0_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[1].set(
	    reinterpret_cast<u64>(exception_intr_1_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[2].set(
	    reinterpret_cast<u64>(exception_intr_2_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[3].set(
	    reinterpret_cast<u64>(exception_intr_3_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[4].set(
	    reinterpret_cast<u64>(exception_intr_4_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[5].set(
	    reinterpret_cast<u64>(exception_intr_5_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[6].set(
	    reinterpret_cast<u64>(exception_intr_6_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[7].set(
	    reinterpret_cast<u64>(exception_intr_7_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[8].set(
	    reinterpret_cast<u64>(exception_intr_8_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[9].set(
	    reinterpret_cast<u64>(exception_intr_9_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[10].set(
	    reinterpret_cast<u64>(exception_intr_10_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[11].set(
	    reinterpret_cast<u64>(exception_intr_11_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[12].set(
	    reinterpret_cast<u64>(exception_intr_12_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[13].set(
	    reinterpret_cast<u64>(exception_intr_13_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[14].set(
	    reinterpret_cast<u64>(exception_intr_14_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[15].set(
	    reinterpret_cast<u64>(exception_intr_15_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[16].set(
	    reinterpret_cast<u64>(exception_intr_16_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[17].set(
	    reinterpret_cast<u64>(exception_intr_17_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[18].set(
	    reinterpret_cast<u64>(exception_intr_18_handler),
	    8 * 1, 0, 0, idte::TRAP);
	idt_vec[19].set(
	    reinterpret_cast<u64>(exception_intr_19_handler),
	    8 * 1, 0, 0, idte::TRAP);

	for (int i = 20; i < 256; i++) {
		idt_vec[i].disable();
	}

	intr_update();

	kern_output* kout = kern_get_out();
	kout->PutStr("convert_vadr_to_padr(idt) = ")
	->PutU64Hex(convert_vadr_to_padr(&idt_vec[255]))
	->PutC('\n');
}

void intr_set_handler(int intr, intr_handler handler)
{
	idt_vec[intr].set(
	    reinterpret_cast<u64>(handler),
	    8 * 1, 0, 0, idte::INTR);

	kern_get_out()->PutStr("idt[")->PutU32Hex(intr)->
	PutStr("] = ")->PutU64Hex(idt_vec[intr].get(1))->PutC(' ')->
	PutU64Hex(idt_vec[intr].get(0))->PutC('\n');

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

	for (;;)
		native::hlt();
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
