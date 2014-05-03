/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <acpi_ctl.hh>
#include <bootinfo.hh>
#include <core/intr_ctl.hh>
#include <core/log.hh>
#include <core/string.hh>
#include <native_cpu_node.hh>
#include <global_vars.hh>
#include <irq_ctl.hh>
#include "kerninit.hh"
#include <mem_io.hh>
#include <mempool_ctl.hh>
#include <native_ops.hh>
#include "native_process.hh"
#include <native_thread.hh>
#include <new_ops.hh>
#include <vga.hh>

#include <clock_src.hh>
#include <timer_ctl.hh>

using namespace x86;

void dump_build_info(output_buffer& ob);
extern char _binary_arch_x86_64_kernel_ap_boot_bin_start[];
extern char _binary_arch_x86_64_kernel_ap_boot_bin_size[];

void test(void*);
bool test_init();
io_node* create_serial();
u64 get_clock();
u64 usecs_to_count(u64 usecs);


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

namespace {
void timer_handler(message* msg)
{
	log()("MSG:")(msg)();
	timer_message* tmsg = static_cast<timer_message*>(msg);
	//tmsg->nanosec_delay = 1000000000;
	global_vars::core.timer_ctl_obj->set_timer(tmsg);
}

cause::t create_first_process()
{
	native_process* pr =
	    new (mem_alloc(sizeof (native_process))) native_process;
	pr->init();

	const bootinfo::module* bundle =
	    reinterpret_cast<const bootinfo::module*>
	    (get_info(bootinfo::TYPE_BUNDLE));
	if (!bundle) {
		log()("no bundle")();
		return cause::FAIL;
	}

	// load text start
	uptr padr;
	cause::t r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &padr);
	if (is_fail(r)) {
		return r;
	}

	void* vadr = arch::map_phys_adr(padr, arch::page::PHYS_L1_SIZE);

	void* src =
	    arch::map_phys_adr(bundle->mod_start, bundle->mod_bytes);

	mem_copy(bundle->mod_bytes, src, vadr);

	//TODO:It is a magic number
	pr->ref_ptbl().set_page(0x00100000, padr,
	    arch::page::PHYS_L1,
	    arch::pte::P | arch::pte::US | arch::pte::A);
	// load text end

	// alloc stack start
	r = get_cpu_node()->page_alloc(arch::page::PHYS_L1, &padr);
	if (is_fail(r)) {
		return r;
	}

	pr->ref_ptbl().set_page(UPTR(0x00007ffffffff000), padr,
	    arch::page::PHYS_L1,
	    arch::pte::P | arch::pte::RW | arch::pte::US |
	    arch::pte::A | arch::pte::D);
	// alloc stack end

	auto thr = create_thread(get_cpu_node(), 0x00100000, 0);
	if (is_fail(thr)) {
		return thr.r;
	}
	native_thread* t = thr.value;
	t->ref_regset()->cr3 = arch::unmap_phys_adr(
	    pr->ref_ptbl().get_toptable(), arch::page::PHYS_L1);
	t->ref_regset()->rsp = 0x0000800000000000;
	t->ref_regset()->cs = 0x20 + 3;
	t->ref_regset()->ds = 0x18 + 3;
	t->ref_regset()->es = 0x18 + 3;
	t->ref_regset()->fs = 0x18 + 3;
	t->ref_regset()->gs = 0x18 + 3;
	t->ref_regset()->ss = 0x18 + 3;

	const uptr thread_private_info =
	   reinterpret_cast<uptr>(t) + t->stack_bytes - sizeof (native_thread*);

	t->thread_private_info = thread_private_info;

	// thread_private_info
	// カーネル空間内のスタックポインタとして使えるアドレスを指す
	// そのアドレスから native_thread 自身を参照できるようにする
	*reinterpret_cast<native_thread**>(thread_private_info) = t;

	// スタックへレジスタを退避するときの都合で、レジスタ１つ分ずらしておく
	t->set_thread_private_info(thread_private_info - sizeof (uptr));

	t->ready();

	return cause::OK;
}

}  // namespace

text_vga vga_dev;
extern "C" int kern_init(u64 bootinfo_adr)
{
	global_vars::arch.bootinfo = reinterpret_cast<void*>(bootinfo_adr);
	global_vars::core.log_target_objs = 0;

	cause::t r = cpu_page_init();
	if (is_fail(r))
		return r;

	preempt_disable();

	global_vars::arch.bootinfo =
	    arch::map_phys_adr(bootinfo_adr, bootinfo::MAX_BYTES);

	r = mempool_init();
	if (is_fail(r))
		return r;

	r = log_init();
	if (is_fail(r))
		return r;

	//vga_dev.init(80, 25, (void*)0xb8000);
	vga_dev.init(80, 25, arch::map_phys_adr(0xb8000, 4096));
	log_install(0, &vga_dev);
	log_install(1, &vga_dev);

	r = mem_io_setup();
	if (is_fail(r))
		return r;

	disable_intr_from_8259A();

	r = x86::thread_ctl_setup();
	if (is_fail(r))
		return r;

	r = x86::native_process_init();
	if (is_fail(r))
		return r;

	r = x86::cpu_ctl_setup();
	if (is_fail(r))
		return r;

	r = x86::cpu_setup();
	if (is_fail(r))
		return r;

	r = create_boot_thread();
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

	r = get_native_cpu_node()->start_message_loop();
	if (is_fail(r))
		return r;

	const int MEMLOG_SIZE = 8192 - 16;
	void* memlog_buffer = mem_alloc(MEMLOG_SIZE);
	if (memlog_buffer) {
		io_node* memio =
		    new (mem_alloc(sizeof (ringed_mem_io)))
		    ringed_mem_io(MEMLOG_SIZE, memlog_buffer);

		log_install(2, memio);

		global_vars::core.memlog_buffer = memlog_buffer;
	}
log(1)("memlog:")(memlog_buffer)(" size:0x").x(MEMLOG_SIZE);

	preempt_enable();

	io_node* serial = create_serial();
	log_install(0, serial);

	const bootinfo::log* bootlog =
	    static_cast<const bootinfo::log*>(get_info(bootinfo::TYPE_LOG));
	if (bootlog) {
		log().write(bootlog->info_bytes - sizeof *bootlog, bootlog->log);
	}

	r = acpi_init();
	if (is_fail(r))
		return r;

	r = timer_setup();
	if (is_fail(r))
		return r;

	const int n = 3;
	timer_message* tmsg = new (mem_alloc(sizeof (timer_message[n]))) timer_message[n];

	for (u64 i = 0; i < n; ++i) {
		tmsg[i].nanosec_delay = (i+1) * 1000000000;
		tmsg[i].handler = timer_handler;
		global_vars::core.timer_ctl_obj->set_timer(&tmsg[i]);
	}

	log lg;
	global_vars::core.timer_ctl_obj->dump(lg);

	tick_time tt;
	get_jiffy_tick(&tt);
	log()("tick=").u(tt)();

	/*
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
	*/

	r = mempool_post_setup();
	if (is_fail(r))
		return r;

	{
		log ob;
		dump_build_info(ob);
	}

	////// start:boot memoryのテスト
	const bootinfo::adr_map* adr_map =
	    reinterpret_cast<const bootinfo::adr_map*>
	    (get_info(bootinfo::TYPE_ADR_MAP));
	const bootinfo::mem_alloc* mem_alloc =
	    reinterpret_cast<const bootinfo::mem_alloc*>
	    (get_info(bootinfo::TYPE_MEM_ALLOC));
	const bootinfo::mem_work* mem_work =
	    reinterpret_cast<const bootinfo::mem_work*>
	    (get_info(bootinfo::TYPE_MEM_WORK));

	log()("ADR_MAP:")();
	u32 read_bytes = sizeof *adr_map;
	for (int i = 0; ; ++i) {
		if (read_bytes >= adr_map->info_bytes)
			break;

		const bootinfo::adr_map::entry& e = adr_map->entries[i];
		log()("adr:").x(e.adr, 16)
		     ("  bytes:").x(e.bytes, 16)
		     ("  type:").x(e.type)
		     ("  zero:").x(e.zero)();

		read_bytes += sizeof adr_map->entries[i];
	}

	log()("MEM_ALLOC:")();
	read_bytes = sizeof *mem_alloc;
	for (int i = 0; ; ++i) {
		if (read_bytes >= mem_alloc->info_bytes)
			break;

		const bootinfo::mem_alloc::entry& e = mem_alloc->entries[i];
		log()("adr:").x(e.adr, 16)
		     ("  bytes:").x(e.bytes, 16)();

		read_bytes += sizeof mem_alloc->entries[i];
	}

	log()("MEM_WORK:")();
	read_bytes = sizeof *mem_work;
	for (int i = 0; ; ++i) {
		if (read_bytes >= mem_work->info_bytes)
			break;

		const bootinfo::mem_work::entry& e = mem_work->entries[i];
		log()("adr:").x(e.adr, 16)
		     ("  bytes:").x(e.bytes, 16)();

		read_bytes += sizeof mem_work->entries[i];
	}
	////// end

	log()("test_init() : ").u(test_init())();

	auto thr = create_thread(get_cpu_node(), test, 0);
	if (is_fail(thr)) {
		log()(SRCPOS)("create_thread() failed");
		return thr.r;
	}
	native_thread* t = thr.value;
	t->ready();

	r = create_first_process();
	if (is_fail(r)) {
		log()("create_first_process() failed")();
	}
	log()("create_first_process() succeeded")();

	get_native_cpu_node()->exit_boot_thread();
	sleep_current_thread();

	return 0;
}

