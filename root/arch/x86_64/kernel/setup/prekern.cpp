/**
 * @file    arch/x86_64/kernel/setup/prekern.cpp
 * @version 0.0.0.1
 * @author  Kato.T
 *
 * @brief   Kernel extend phase before execute.
 */
// (C) Kato.T 2010

#include "mem.hpp"
#include "term.hpp"


namespace {


const char* acpi_memtype(int type)
{
	const static char* type_name[] = {
		"unknown", "memory", "reserved", "acpi", "nvs", "unusuable" };

	if (1 <= type && type <= 5) {
		return type_name[type];
	}
	else {
		return type_name[0];
	}
}

}  // End of anonymous namespace

extern "C" int prekernel()
{
	video_term vt;
	vt.init(
		setup_data<_u32>(SETUP_DISP_WIDTH),
		setup_data<_u32>(SETUP_DISP_HEIGHT),
		setup_data<_u32>(SETUP_DISP_VRAM));
	vt.set_cur(
		setup_data<_u32>(SETUP_DISP_CURROW),
		setup_data<_u32>(SETUP_DISP_CURCOL));

	term_chain tc;
	tc.add_term(&vt);

	tc.puts("DISPLAY : ")
	->putu64(setup_data<_u32>(SETUP_DISP_WIDTH))
	->putc('x')
	->putu64(setup_data<_u32>(SETUP_DISP_HEIGHT))
	->putc('\n');

	tc.puts("Memorymap by ACPI : \n");

	const acpi_memmap* memmap_buf =
		setup_ptr<const acpi_memmap>(SETUP_MEMMAP);
	const int memmaps = setup_data<_u32>(SETUP_MEMMAP_COUNT);
	if (memmaps < 0) {
		return 0;
	}
	for (int i = 0; i < memmaps; i++) {
		const _u64 base = memmap_buf[i].base;
		const _u64 length = memmap_buf[i].length;
		const _u32 type = memmap_buf[i].type;
		tc.putu64(i)
		->puts(" : ")
		->putu64x(base)
		->puts(" - ")
		->putu64x(base + length)
		->putc(' ')
		->puts(acpi_memtype(type))
		->putc('\n');
	}

	memmgr mm;
	memmgr_init(&mm);
	tc.puts("mem inited\n");

	void* p = memmgr_alloc(&mm, 16);
	tc.puts("mem = ")->putu64x((_u64)p)->putc('\n');

	return 0;
}
