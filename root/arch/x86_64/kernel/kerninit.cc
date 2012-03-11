/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010-2012 KATO Takeshi
//

#include "arch.hh"
#include "bootinfo.hh"
#include "global_vars.hh"
#include "interrupt_control.hh"
#include "irq_ctl.hh"
#include "kerninit.hh"
#include "log.hh"
#include "memory_allocate.hh"
#include "mempool_ctl.hh"
#include "native_ops.hh"

#include "setupdata.hh"
#include "boot_access.hh"
#include "event_queue.hh"
#include "placement_new.hh"
#include "vga.hh"

#include "memcell.hh"

#include "mempool.hh"
#include <processor.hh>


extern char _binary_arch_x86_64_kernel_ap_boot_bin_start[];
extern char _binary_arch_x86_64_kernel_ap_boot_bin_size[];
thread* ta;
thread* tb;
void testA(void* p)
{
	processor* proc = get_current_cpu();
	thread_ctl& tc = proc->get_thread_ctl();
	for (u32 i=0;;++i) {
		log()("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
		if (i==0){
			tc.ready_thread(tb);
			proc->sleep_current_thread();
		}
	}
}

void testB(void* p)
{
	processor* proc = get_current_cpu();
	thread_ctl& tc = proc->get_thread_ctl();
	for (u32 i=0;;++i) {
		log()("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
		if (i==0){
			tc.ready_thread(ta);
			proc->sleep_current_thread();
		}
	}
}

void test(void*);
bool test_init();
void cpu_test();
file* create_serial();
file* attach_console(int w, int h, u64 vram_adr);
void drive();
void lapic_dump();
void serial_dump(void*);
cause::stype slab_init();
bool hpet_init();
u64 get_clock();
u64 usecs_to_count(u64 usecs);

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

void apentry()
{
	log(1)("apentry()")();
	for(;;)native::hlt();
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

	thread_ctl& tc =
	    global_vars::gv.logical_cpu_obj_array[0].get_thread_ctl();

	tc.set_event_thread(tc.get_running_thread());

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

	hpet_init();
	log()("clock=").u(get_clock())();
	log()("clock=").u(get_clock())();

	const mpspec* mps = get_current_cpu()->get_shared()->get_mpspec();
	mpspec::processor_iterator proc_itr(mps);
	for (;;) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		log()("cpu : ").u(pe->localapic_id)();
	}

	lapic_post_init_ipi();
	u64 initipi_clock = get_clock();

	struct ap_param {
		u64 pml4;
		u64 entry_point;
		void* stack;
	};

	log(1)("apboot_start=").p(_binary_arch_x86_64_kernel_ap_boot_bin_start)();
	log(1)("apboot_size=").p(_binary_arch_x86_64_kernel_ap_boot_bin_size)();
	u8* apboot_start = (u8*)_binary_arch_x86_64_kernel_ap_boot_bin_start;
	uptr apboot_size = (uptr)_binary_arch_x86_64_kernel_ap_boot_bin_size;
	u8* dest = (u8*)arch::map_phys_adr(0x1000, apboot_size);
	for (uptr i = 0; i < apboot_size; ++i) {
		dest[i] = apboot_start[i];
	}
	ap_param* app = (ap_param*)up_align<uptr>((uptr)&dest[apboot_size], 8);
	app->pml4 = native::get_cr3();
	app->entry_point = (u64)apentry;
	mempool* stackmp = mempool_create_shared(0x2000);
	u8* apstack = (u8*)stackmp->alloc();
	app->stack = apstack + 0x2000;

	u64 _10ms = usecs_to_count(10000);
	u64 i;
	for (i = 0; ; ++i) {
		if (get_clock() >= (initipi_clock + _10ms))
			break;
	}
	log(1)("loop count:").u(i)();
	lapic_post_startup_ipi(0x01);
	u64 startupipi_clock = get_clock();
	u64 _200ms = usecs_to_count(200000);
	for (i = 0; ; ++i) {
		if (get_clock() >= (startupipi_clock + _200ms))
			break;
	}
	log(1)("loop count:").u(i)();

//	cpu_test();
//	serial_dump(serial);
	log()("test_init() : ").u(test_init())();
	//test();

	thread* t;
	tc.create_thread(test, 0, &t);
	tc.ready_thread(t);
/*
	tc.create_thread(testA, 0, &t);
	ta = t;
	tc.ready_thread(t);
	tc.create_thread(testB, 0, &t);
	tb = t;
	tc.ready_thread(t);
*/
	serial->sync = true;
	return 0;
}

extern "C" void kern_service()
{
	drive();
}

