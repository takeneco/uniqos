// @file   arch/x86_64/kernel/kerninit.cpp
// @author Kato Takeshi
// @brief  Call kernel initialize funcs.
//
// (C) 2010 Kato Takeshi.

#include "kerninit.hh"

#include "output.hh"
#include "setupdata.hh"


namespace {


}  // End of anonymous namespace


KernOutput* kout;
extern int kern_tail_addr;


extern "C" int KernInit()
{
	cpu_init();

	VideoOutput vo;
	kout = &vo;

	u32 width, height, vram, row, col;
	SetupGetCurrentDisplayMode(&width, &height, &vram);
	SetupGetCurrentDisplayCursor(&row, &col);
	//vo.Init(width, height, vram);
	vo.Init(width, height,
	    0xffffffffc0200000 + 0xb8000);
	vo.SetCur(row, col);

	IDTInit();

	char c = 'a';
	IOVector v;
	v.Bytes = 1;
	v.Address = &c;
	vo.Write(&v, 1, 0);

	vo.PutStr("width=")->PutUDec(width)->
	   PutStr(":height=")->PutUDec(height)->
	   PutStr(":vram=")->PutU32Hex(vram)->
	   PutC('\n');
	vo.PutStr("xxxxxxxxxxxx\nxxxxxxxxxxxxx\nxxxxxxxxxxxxxx\nxxxxxxxxxxxxxxxxx\n");
	vo.PutStr("1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n");
	vo.PutStr("kern_tail_addr = ")->
	   PutU64Hex(reinterpret_cast<u64>(&kern_tail_addr))->
	   PutC('\n');
	vo.PutStr("&VideoOutput = ")->
	   PutU64Hex(reinterpret_cast<u64>(&vo))->
	   PutC('\n');
	//vo.PutSDec(-12345);
	//vo.PutU64Hex(0xdeadbeefdeadbeef);

	return 0;
}
