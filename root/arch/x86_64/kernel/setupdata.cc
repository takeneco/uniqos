// @file   arch/x86_64/kernel/setupdata.cpp
// @author Kato Takeshi
// @brief  Access to setup data.
//
// (C) Kato Takeshi 2010

#include "setupdata.hh"

#include "setup/access.hh"


void SetupGetCurrentDisplayMode(u32* Width, u32* Height, u32* VRam)
{
	*Width  = setup_get_value<u32>(SETUP_DISP_WIDTH);
	*Height = setup_get_value<u32>(SETUP_DISP_HEIGHT);
	*VRam   = setup_get_value<u32>(SETUP_DISP_VRAM);
}

void SetupGetCurrentDisplayCursor(u32* Row, u32* Col)
{
	*Row = setup_get_value<u32>(SETUP_DISP_CURROW);
	*Col = setup_get_value<u32>(SETUP_DISP_CURCOL);
}

