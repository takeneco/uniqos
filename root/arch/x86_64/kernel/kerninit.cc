/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "kerninit.hh"
#include "output.hh"
#include "setupdata.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"

#include "setupdata.hh"
#include "setup/memdump.hh"


void test();

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



void tmp_debug();

kern_output* kout;
extern int kern_tail_addr;

kern_output* kern_get_out()
{
	return kout;
}

void func()
{
	dump()("func")(1)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	dump()("func")(2)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	dump()("func")(3)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
	dump()("func")(4)();
	asm volatile ("callq task_switch" : : "a"(&ts2), "c"(&ts1));
}

extern "C" int kern_init()
{
	kout = 0;

	cpu_init();

	VideoOutput vo;
	kout = &vo;

	u32 width, height, vram, row, col;
	setup_get_display_mode(&width, &height, &vram);
	setup_get_display_cursor(&row, &col);
	vo.Init(width, height,
	    arch::PHYSICAL_MEMMAP_BASEADR + 0xb8000);
	vo.SetCur(row, col);

	vo.PutStr("width=")->PutUDec(width)->
	   PutStr(":height=")->PutUDec(height)->
	   PutStr(":vram=")->PutU32Hex(vram)->
	   put_c('\n');

	serial_kout_init();
	//serial_output_init();

	dump_init(&serial_get_kout());
	//serial_output* com1 = serial_get_out(0);
	//kout=com1;

	setup_memmgr_dumpdata* map;
	u32 map_num;
	setup_get_used_memdump(&map, &map_num);
	vo.put_str("used:\n");
	for (u32 i = 0; i < map_num; ++i) {
		vo.put_u64hex(map[i].head)->put_c(':')->
		   put_u64hex(map[i].head + map[i].bytes)->put_c('\n');
	}

	setup_get_free_memdump(&map, &map_num);
	vo.put_str("free:\n");
	for (u32 i = 0; i < map_num; ++i) {
		vo.put_u64hex(map[i].head)->put_c(':')->
		   put_u64hex(map[i].head + map[i].bytes)->put_c('\n');
	}

	cause::stype r = arch::pmem::init();

	setup_get_free_memdump(&map, &map_num);
	vo.put_str("free:\n");
	for (u32 i = 0; i < map_num; ++i) {
		vo.put_u64hex(map[i].head)->put_c(':')->
		   put_u64hex(map[i].head + map[i].bytes)->put_c('\n');
	}

	vo.put_str("pmem::init() = ")->put_udec(r)->put_endl();

	memory::init();
	arch::apic_init();

	dump()("ts1=")(&ts1)()("ts2=")(&ts2)();
	create_thread(&ts2, func);
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	dump()("kerninit")(1)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	dump()("kerninit")(2)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));
	dump()("kerninit")(3)();
	asm volatile ("callq task_switch" : : "a"(&ts1), "c"(&ts2));

	test();

	return 0;
}
