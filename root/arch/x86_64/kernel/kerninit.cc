/// @file  kerninit.cc
/// @brief Call kernel initialize funcs.
//
// (C) 2010 KATO Takeshi
//

#include "kerninit.hh"

#include "output.hh"
#include "setupdata.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"

#include "setupdata.hh"
#include "setup/memdump.hh"

namespace {


}  // End of anonymous namespace

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
	//vo.Init(width, height, vram);
	vo.Init(width, height,
//	    0xffffffffc0200000 + 0xb8000);
	    0xffff800000000000L + 0xb8000);
	vo.SetCur(row, col);

	vo.PutStr("width=")->PutUDec(width)->
	   PutStr(":height=")->PutUDec(height)->
	   PutStr(":vram=")->PutU32Hex(vram)->
	   PutC('\n');
	vo.PutStr("kern_tail_addr = ")->
	   PutU64Hex(reinterpret_cast<u64>(&kern_tail_addr))->
	   PutC('\n');
	vo.PutStr("&VideoOutput = ")->
	   PutU64Hex(reinterpret_cast<u64>(&vo))->
	   PutC('\n');
	vo.put_str("%ss = ")->
	   PutU16Hex(native::get_ss())->
	   PutC('\n');

	serial_output_init();

	serial_output* com1 = serial_get_out(0);
com1->PutStr("xyz");
kout=com1;

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

	char* p = (char*)memory::alloc(4056);
	vo.put_str("memory::alloc(1) = ")->put_u64hex((u64)p)->put_c('\n');
	*p = 'x';

	p = (char*)memory::alloc(4056);
	vo.put_str("memory::alloc(1) = ")->put_u64hex((u64)p)->put_c('\n');
	*p = 'x';

	p = (char*)memory::alloc(1);
	vo.put_str("memory::alloc(1) = ")->put_u64hex((u64)p)->put_c('\n');
	*p = 'x';

	arch::apic_init();

	tmp_debug();
	return 0;
}
