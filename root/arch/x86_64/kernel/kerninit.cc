// @file   arch/x86_64/kernel/kerninit.cpp
// @author Kato Takeshi
// @brief  Call kernel initialize funcs.
//
// (C) Kato Takeshi 2010

#include "kerninit.hh"

#include "output.hh"
#include "setupdata.hh"


namespace {


}  // End of anonymous namespace


VideoOutput vo;
KernOutput* kout;

extern "C" int KernInit()
{
	cpu_init();

	kout = &vo;

	u32 width, height, vram, row, col;
	SetupGetCurrentDisplayMode(&width, &height, &vram);
	SetupGetCurrentDisplayCursor(&row, &col);
	vo.Init(width, height, vram);
	vo.SetCur(row, col);

	char c = 'a';
	IOVector v;
	v.Bytes = 1;
	v.Address = &c;
	vo.Write(&v, 1, 0);

	return 0;
}
