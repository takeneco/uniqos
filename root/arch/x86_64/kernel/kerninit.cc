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
#include <processor.hh>


void test(void*);
bool test_init();
void cpu_test();
file* create_serial();
file* attach_console(int w, int h, u64 vram_adr);
void drive();
void lapic_dump();
void serial_dump(void*);
cause::stype slab_init();

namespace {

}  // End of anonymous namespace


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

	arch::apic_init();

	file* serial = create_serial();
	log_init(0, serial);
	serial->sync = false;

	const bootinfo::log* bootlog =
	    reinterpret_cast<const bootinfo::log*>
	    (get_bootinfo(bootinfo::TYPE_LOG));
	if (bootlog) {
		log().write(bootlog->log, bootlog->size - sizeof *bootlog);
	}
log(1)("eee")();

//	cpu_test();
//	serial_dump(serial);
	log()("test_init() : ").u(test_init())();
	//test();

	thread_ctl& tc =
	    global_vars::gv.logical_cpu_obj_array[0].get_thread_ctl();

	tc.set_event_thread(tc.get_running_thread());

	thread* t;
	tc.create_thread(test, 0, &t);
	tc.ready_thread(t);

	serial->sync = true;
	return 0;
}

extern "C" void kern_service()
{
	drive();
}

