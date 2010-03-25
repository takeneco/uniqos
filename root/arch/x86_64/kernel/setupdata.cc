// @file   arch/x86_64/kernel/setupdata.cpp
// @author Kato Takeshi
// @brief  Access to setup data.
//
// (C) Kato Takeshi 2010

#include "setupdata.hh"

#include "setup/access.hh"


void SetupGetCurrentDisplayMode(u32* Width, u32* Height, u32* VRam)
{
	*Width  = SetupGetValue<u32>(SETUP_DISP_WIDTH);
	*Height = SetupGetValue<u32>(SETUP_DISP_HEIGHT);
	*VRam   = SetupGetValue<u32>(SETUP_DISP_VRAM);
}

void SetupGetCurrentDisplayCursor(u32* Row, u32* Col)
{
	*Row = SetupGetValue<u32>(SETUP_DISP_CURROW);
	*Col = SetupGetValue<u32>(SETUP_DISP_CURCOL);
}

