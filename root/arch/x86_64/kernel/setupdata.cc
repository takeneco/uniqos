// @file   arch/x86_64/kernel/setupdata.cpp
// @author Kato Takeshi
// @brief  Access to setup data.
//
// (C) Kato Takeshi 2010

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

