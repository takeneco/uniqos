/// @author KATO Takeshi
/// @brief  Access to setup data.
//
// (C) KATO Takeshi 2010
//
/// @todo 物理メモリマップを使うようにする。

#include "setupdata.hh"

#include "setup/access.hh"


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

