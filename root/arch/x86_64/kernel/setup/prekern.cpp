/**
 * @file    arch/x86_64/kernel/setup/prekern.cpp
 * @author  Kato.T
 * @brief   Kernel extend phase before execute.
 *
 * (C) Kato Takeshi 2010
 */

#include "mem.hpp"
#include "pagetable.hpp"
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

extern "C" int prekernel(_u64 kern_size, _u8* compressed_kern)
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

	void* p2 = memmgr_alloc(&mm, 16);
	tc.puts("mem = ")->putu64x((_u64)p2)->putc('\n');

	arch::pte* pdpte = reinterpret_cast<arch::pte*>(KERN_PDPTE_PHADR);
	_u64 pde_adr = KERN_PDE_PHADR;

	pdpte[255].set(pde_adr,
		arch::pte::P | arch::pte::RW | arch::pte::PS | arch::pte::G);

	// Kernel text body

	arch::pte* pde = reinterpret_cast<arch::pte*>(pde_adr);
	void* p = memmgr_alloc(&mm, 0x200000, 0x200000);
	// 0x....ffffc0000000
	pde[0].set(reinterpret_cast<_u64>(p),
		arch::pte::P | arch::pte::RW | arch::pte::PS | arch::pte::G);

	// Kernel stack memory

	p = memmgr_alloc(&mm, 0x200000, 0x200000);
	// 0x....ffffffe00000
	pde[255].set(reinterpret_cast<_u64>(p),
		arch::pte::P | arch::pte::RW | arch::pte::PS | arch::pte::G);

	pde_adr += 8 * 512;

	p = memmgr_alloc(&mm, 16);
	tc.puts("mem = ")->putu64x((_u64)p)->putc('\n');

	asm ("invlpg " TOSTR(KERN_FINAL_ADR));

	return 0;
}
