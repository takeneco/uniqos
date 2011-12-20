/// @file  setupdata.cc
/// @brief  Access to setup data.
//
// (C) 2010-2011 KATO Takeshi
//
/// @todo 物理メモリマップを使うようにする。

#include "global_vars.hh"
#include "setupdata.hh"

#include "boot_access.hh"


void setup_get_display_mode(u32* width, u32* height, u32* vram)
{
	*width  = setup_get_value<u32>(SETUP_DISP_WIDTH);
	*height = setup_get_value<u32>(SETUP_DISP_HEIGHT);
	*vram   = setup_get_value<u32>(SETUP_DISP_VRAM);
}

void setup_get_display_cursor(u32* row, u32* col)
{
	*row = setup_get_value<u32>(SETUP_DISP_CURROW);
	*col = setup_get_value<u32>(SETUP_DISP_CURCOL);
}

void setup_get_free_memdump(setup_memory_dumpdata** freedump, u32* num)
{
	*freedump = setup_get_ptr<setup_memory_dumpdata>(SETUP_FREEMEM_DUMP);
	*num = setup_get_value<u32>(SETUP_FREEMEM_DUMP_COUNT);
}

void setup_get_used_memdump(setup_memory_dumpdata** useddump, u32* num)
{
	*useddump = setup_get_ptr<setup_memory_dumpdata>(SETUP_USEDMEM_DUMP);
	*num = setup_get_value<u32>(SETUP_USEDMEM_DUMP_COUNT);
}

void setup_get_mp_info(u8** ptr)
{
	*ptr = setup_get_ptr<u8>(SETUP_MP_FLOATING_POINTER);
}


#include "bootinfo.hh"
#include "multiboot2.h"


const void* get_bootinfo(u32 tag_type)
{
	const u8* bootinfo =
	    reinterpret_cast<const u8*>(global_vars::gv.bootinfo);

	const u32 total_size = *reinterpret_cast<const u32*>(bootinfo);

	u32 read = sizeof (u32) * 2;

	for (;;) {
		const multiboot_tag* tag =
		    reinterpret_cast<const multiboot_tag*>(&bootinfo[read]);

		if (tag->type == tag_type)
			return tag;

		read += up_align<u32>(tag->size, MULTIBOOT_TAG_ALIGN);
		if (read >= total_size)
			break;
	}

	return 0;
}

