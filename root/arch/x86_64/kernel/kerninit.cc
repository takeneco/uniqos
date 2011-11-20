/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch.hh"
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


void test();
void cpu_test();
file_interface* create_serial();
file_interface* attach_console(int w, int h, u64 vram_adr);
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

text_vga vga_dev;
extern "C" int kern_init()
{
	vga_dev.init(80, 25, (void*)0xb8000);
	log_file vgalog(&vga_dev);
	log_init(&vgalog);

	cause::stype r = arch::page::init();
	if (r != cause::OK)
		return r;

	cpu_init();

	native::sti();

	global_vars::gv.irq_ctl_obj->init();

log()("ddd")();
for(;;)native::hlt();
	global_vars::gv.intr_ctl_obj->init();
log()("eee")();
for(;;)native::hlt();

	global_vars::gv.events =
		new (memory::alloc(sizeof (event_queue))) event_queue;

	arch::apic_init();

	slab_init();

	file_interface* serial = create_serial();
	log_file lf(serial);
	log_init(&lf);

	{
	char* setup_log;
	u32 setup_log_cur, setup_log_size;
	setup::get_log(&setup_log, &setup_log_cur, &setup_log_size);
	log().write(setup_log,
	    setup_log_cur < setup_log_size ? setup_log_cur : setup_log_size);
	}

for (int i = 0; i < 4096; ++i) {
	log()("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")();
}

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
	//test();

	return 0;
}

extern "C" void kern_service()
{
	drive();
}

