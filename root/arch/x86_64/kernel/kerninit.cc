/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch.hh"
#include "bootinfo.hh"
#include "core_class.hh"
#include "global_vars.hh"
#include "kerninit.hh"
#include "output.hh"
#include "memcache.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"

#include "setupdata.hh"
#include "boot_access.hh"
#include "event_queue.hh"
#include "placement_new.hh"
#include "vga.hh"

#include "memcell.hh"

#include "mempool.hh"
#include "regset.hh"


void test();
bool test_init();
void cpu_test();
file* create_serial();
file* attach_console(int w, int h, u64 vram_adr);
void drive();
void lapic_dump();
void serial_dump(void*);
cause::stype slab_init();

#include "task.hh"
extern "C" void task_switch(thread_state*, thread_state*);
void create_thread(thread_state* ts, void (*entry)())
{
	u64 eflags;
	asm volatile ("pushfq;popq %0": "=r"(eflags));
	ts->eflags = eflags;

	u64* stack = (u64*)memory::alloc(sizeof (u64) * 256);
	stack[255] = (u64)entry;
	ts->rsp = (u64)&stack[255];
}
thread_state ts1, ts2;

namespace {

}  // End of anonymous namespace


kern_output* kout_;
extern int kern_tail_addr;

kern_output* kern_get_out()
{
	return kout_;
}

void func()
{
	log()("func")(1)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	log()("func")(2)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	log()("func")(3)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	log()("func")(4)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
}

void disable_intr_from_8259A()
{
	enum {
		PIC0_ICW1 = 0x20,
		PIC0_OCW1 = 0x21,
		PIC0_ICW2 = 0x21,
		PIC0_ICW3 = 0x21,
		PIC0_ICW4 = 0x21,
		PIC1_ICW1 = 0xa0,
		PIC1_OCW1 = 0xa1,
		PIC1_ICW2 = 0xa1,
		PIC1_ICW3 = 0xa1,
		PIC1_ICW4 = 0xa1,
	};

	native::outb(0xff, PIC0_OCW1);
	native::outb(0xff, PIC1_OCW1);

	native::outb(0x11, PIC0_ICW1);
	native::outb(0xf8, PIC0_ICW2); // 使わなさそうなベクタにマップしておく
	native::outb(1<<2, PIC0_ICW3);
	native::outb(0x01, PIC0_ICW4);

	native::outb(0x11, PIC1_ICW1);
	native::outb(0xf8, PIC1_ICW2);
	native::outb(2   , PIC1_ICW3);
	native::outb(0x01, PIC1_ICW4);

	native::outb(0xff, PIC0_OCW1);
	native::outb(0xff, PIC1_OCW1);
}

extern "C" void switch_regset(regset* r1, regset* r2);

thread* t1;
thread* t2;

void thread2()
{
	log()("xxx")();
	switch_regset(&t1->rs, &t2->rs);
}

text_vga vga_dev;
extern "C" int kern_init(u64 bootinfo_adr)
{
	global_vars::gv.bootinfo = reinterpret_cast<void*>(bootinfo_adr);

	vga_dev.init(80, 25, (void*)0xb8000);
	log_init(0, &vga_dev);
	log_init(1, &vga_dev);

	cause::stype r = page_ctl_init();
	if (r != cause::OK)
		return r;

	global_vars::gv.bootinfo =
	    arch::map_phys_adr(bootinfo_adr, bootinfo::MAX_BYTES);

	global_vars::gv.mempool_ctl_obj->init();
	if (r != cause::OK)
		return r;

	disable_intr_from_8259A();

	r = cpu_init();
	if (r != cause::OK)
		return r;

	native::sti();

	r = global_vars::gv.irq_ctl_obj->init();
	if (r != cause::OK)
		return r;

	r = global_vars::gv.intr_ctl_obj->init();
	if (r != cause::OK)
		return r;
log()("eee")();

	arch::apic_init();

	const bootinfo::log* bootlog =
	    reinterpret_cast<const bootinfo::log*>
	    (get_bootinfo(bootinfo::TYPE_LOG));
	if (bootlog) {
		log().write(bootlog->log, bootlog->size - sizeof *bootlog);
	}

	file* serial = create_serial();
	log_init(0, serial);

/*
	log()("ts1=")(&ts1)()("ts2=")(&ts2)();
	create_thread(&ts2, func);
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	log()("kerninit")(1)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	log()("kerninit")(2)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	log()("kerninit")(3)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
*/

//	memcell_test();

//	cpu_test();
//	serial_dump(serial);
	log()("test_init() : ").u(test_init())();
	//test();

	mempool* mp = mempool_create_shared(0x2000);
	t2 = new (mp->alloc()) thread(
	    (u64)thread2, (u64)mp->alloc() + 0x2000);

	t1 = new (mp->alloc()) thread(0, 0);

	log()("zzz")();
	switch_regset(&t2->rs, &t1->rs);
	log()("zzz")();

	return 0;
}

extern "C" void kern_service()
{
	drive();
}

