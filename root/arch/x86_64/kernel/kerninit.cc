/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010-2012 KATO Takeshi
//

#include <acpi_ctl.hh>
#include <bootinfo.hh>
#include <cpu_node.hh>
#include <global_vars.hh>
#include <intr_ctl.hh>
#include <irq_ctl.hh>
#include "kerninit.hh"
#include <log.hh>
#include <mem_io.hh>
#include <mempool_ctl.hh>
#include <native_ops.hh>
#include <new_ops.hh>
#include <vga.hh>


extern char _binary_arch_x86_64_kernel_ap_boot_bin_start[];
extern char _binary_arch_x86_64_kernel_ap_boot_bin_size[];
thread* ta;
thread* tb;
void testA(void* p)
{
	cpu_node* proc = get_cpu_node();
	thread_queue& tc = proc->get_thread_ctl();
	for (u32 i=0;;++i) {
		log()("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
		if (i==0){
			//tc.ready_thread(tb);
			//proc->sleep_current_thread();
		}
	}
}

void testB(void* p)
{
	cpu_node* proc = get_cpu_node();
	thread_queue& tc = proc->get_thread_ctl();
	for (u32 i=0;;++i) {
		log()("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
		if (i==0){
			//tc.ready_thread(ta);
			//proc->sleep_current_thread();
		}
	}
}

void test(void*);
bool test_init();
void cpu_test();
io_node* create_serial();
void drive();
void lapic_dump();
void serial_dump(void*);
bool hpet_init();
u64 get_clock();
u64 usecs_to_count(u64 usecs);
void kern_service(void* param);


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
	global_vars::arch.bootinfo = reinterpret_cast<void*>(bootinfo_adr);

	cause::type r = cpu_page_init();
	if (is_fail(r))
		return r;

	global_vars::arch.bootinfo =
	    arch::map_phys_adr(bootinfo_adr, bootinfo::MAX_BYTES);

	r = mempool_init();
	if (is_fail(r))
		return r;

	r = log_init();
	if (is_fail(r))
		return r;

	vga_dev.init(80, 25, (void*)0xb8000);
	log_install(0, &vga_dev);
	log_install(1, &vga_dev);

	r = mem_io_setup();
	if (is_fail(r))
		return r;
/*
	r = acpi_init();
	if (is_fail(r))
		return r;
*/
	disable_intr_from_8259A();

	r = cpu_common_init();
	if (is_fail(r))
		return r;

	r = cpu_setup();
	if (is_fail(r))
		return r;

	r = intr_setup();
	if (is_fail(r))
		return r;

	r = irq_setup();
	if (is_fail(r))
		return r;

	// TODO: replace
	arch::apic_init();

	thread_queue& tc = get_cpu_node()->get_thread_ctl();
/*	thread* event_thread;
	r = tc.create_thread(&cpu_node::message_loop_entry,
			get_cpu_node(), &event_thread);
	//r = tc.create_thread(&kern_service, 0, &event_thread);
	if (is_fail(r))
		return r;

	tc.wakeup(event_thread);
	tc.set_event_thread(event_thread);
*/
	r = get_cpu_node()->start_message_loop();
	if (is_fail(r))
		return r;

	void* memlog_buffer = mem_alloc(8192);
	if (memlog_buffer) {
		io_node* memio =
		    new (mem_alloc(sizeof (ringed_mem_io)))
		    ringed_mem_io(8192, memlog_buffer);

		log_install(2, memio);

		global_vars::core.memlog_buffer = memlog_buffer;
	}

	native::sti();

	io_node* serial = create_serial();
	log_install(0, serial);
	serial->sync = true;

	const bootinfo::log* bootlog =
	    reinterpret_cast<const bootinfo::log*>
	    (get_bootinfo(bootinfo::TYPE_LOG));
	if (bootlog) {
		log().write(bootlog->size - sizeof *bootlog, bootlog->log);
	}

	/////
	r = acpi_init();
	if (is_fail(r))
		return r;
	/////

	r = timer_setup();
	if (is_fail(r))
		return r;

	r = timer_setup_cpu();
	if (is_fail(r))
		return r;

log()(__FILE__,__LINE__,__func__)();for (;;) native::hlt();

	hpet_init();
	log()("clock=").u(get_clock())();
	log()("clock=").u(get_clock())();


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
	mempool* stackmp;
	r = mempool_acquire_shared(0x2000, &stackmp);
	if (is_fail(r))
		return r;
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

