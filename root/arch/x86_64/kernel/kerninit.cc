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

namespace {

kout* kout_;
unsigned int ko_mask;

}  // End of anonymous namespace

kout& ko(u8 i)
{
	return ko_mask & (1 << i) ? *kout_ : *static_cast<kout*>(0);
}

void ko_set(u8 i, bool mask)
{
	if (mask)
		ko_mask |= 1 << i;
	else
		ko_mask &= ~(1 << i);
}

void tmp_debug();

kern_output* kout;
extern int kern_tail_addr;

kern_output* kern_get_out()
{
	return kout;
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

	kout_ = &serial_get_kout();
	ko_mask = 0x000001;
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

	uptr adrs[64];
	for (int i = 0; i < 1; ++i) {
		r = arch::pmem::alloc_l1page(&adrs[i]);
		if (r != 0) {
			vo.put_str("r=")->put_udec(r)->put_str(",i=")->
			put_udec(i)->put_endl();
		} else {
			vo.put_str("alloc[")->put_udec(i)->put_str("]=")->
			put_u64hex(adrs[i])->put_endl();
		}
	}

	memory::init();
	arch::apic_init();

	void* p[50];
	arch::wait(0x800000);
	for (int i = 0; i < 40; i++) {
		p[i] = memory::alloc(800);
	}
	arch::wait(0x800000);
	for (int i = 0; i < 40; i++) {
		memory::free(p[i]);
	}
	arch::wait(0x800000);
	for (int i = 0; i < 40; i++) {
		void* x = memory::alloc(800);
		ko()(p[i])(':')(x)();
	}

	test();

	tmp_debug();
	return 0;
}
