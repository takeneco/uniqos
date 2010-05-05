// @file   arch/x86_64/kernel/kerninit.cpp
// @author Kato Takeshi
// @brief  Call kernel initialize funcs.
//
// (C) 2010 Kato Takeshi.

#include "kerninit.hh"

#include "output.hh"
#include "setupdata.hh"
#include "native.hh"


namespace {


}  // End of anonymous namespace


kern_output* kout;
extern int kern_tail_addr;

kern_output* kern_get_out()
{
	return kout;
}

extern "C" int kern_init()
{
	kout = 0;

	//cpu_init();

	VideoOutput vo;
	kout = &vo;

	u32 width, height, vram, row, col;
	SetupGetCurrentDisplayMode(&width, &height, &vram);
	SetupGetCurrentDisplayCursor(&row, &col);
	//vo.Init(width, height, vram);
	vo.Init(width, height,
	    0xffffffffc0200000 + 0xb8000);
	vo.SetCur(row, col);
cpu_init();

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
	vo.PutStr("%ss = ")->
	   PutU16Hex(native_get_ss())->
	   PutC('\n');

	serial_output_init();

/*
	int a, b, c;
	a = 0;
	b = 1;
	c = b / a;
*/

	serial_output* com1 = serial_get_out(0);
	vo.PutStr("com1 = ")->PutU64Hex((u64)com1)->PutC('\n');
//	vo.PutStr("c = ")->PutU32Hex(c)->PutC('\n');
//	com1->PutStr("x");
/*
	io_vector iov;
	char c = 'x';
	iov.bytes = 1;
	iov.address = &c;
	com1->write(&iov, 1, 0);
*/
	return 0;
}
