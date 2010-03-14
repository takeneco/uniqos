/**
 * @file    arch/x86_64/kernel/setup/prekern.cpp
 * @author  Kato Takeshi
 * @brief   Kernel extend phase before execute.
 *
 * (C) Kato Takeshi 2010
 */

#include "mem.hpp"
#include "pagetable.hpp"
#include "term.hpp"
#include "lzmadecwrap.hpp"


extern char setup_body_start;
extern char kern_body_start;

term_chain* debug_tc;

namespace {

	const char* acpi_memtype(int type)
	{
		const static char* type_name[] = {
			"unknown", "memory", "reserved",
			"acpi", "nvs", "unusuable" };

		if (1 <= type && type <= 5) {
			return type_name[type];
		}
		else {
			return type_name[0];
		}
	}

	bool kern_extract(memmgr* mm)
	{
		const _u32 setup_size = &kern_body_start - &setup_body_start;

		_u8* const comp_kern_src =
			reinterpret_cast<_u8*>(SETUP_KERN_ADR + setup_size);

		const _u32 comp_kern_size =
			setup_data<_u32>(SETUP_KERNFILE_SIZE) - setup_size;

		const _u32 ext_kern_size = lzma_decode_size(comp_kern_src);

		_u8* const ext_kern_dest =
			reinterpret_cast<_u8*>(KERN_FINAL_VADR);

		return lzma_decode(mm, comp_kern_src, comp_kern_size,
			ext_kern_dest, ext_kern_size);
	}

}  // End of anonymous namespace


/**
 * @brief  Create memory map, Kernel body extract, etc...
 * @retval 0     Succeeds. Kernel body exectable.
 * @retval other Fails.
 */
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

	debug_tc = &tc;

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
		return -1;
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

	// pdpte_base[512 *   0] -> 0x....800.........
	// pdpte_base[512 *   1] -> 0x....808.........
	// pdpte_base[512 * 254] -> 0x....ff0.........
	// pdpte_base[512 * 255] -> 0x....ff8.........
	arch::pte* pdpte_base = reinterpret_cast<arch::pte*>(KERN_PDPTE_PADR);
	_u64 pde_adr = KERN_PDE_PADR;

	arch::pte* pdpte = &pdpte_base[512 * 255];

	// pdpte_base[512 * 255 + 511] -> 0x....ffffc.......
	pdpte[511].set(pde_adr,
		arch::pte::P | arch::pte::RW);

	// Kernel text body

	arch::pte* pde = reinterpret_cast<arch::pte*>(pde_adr);
	char* p1 = (char*)memmgr_alloc(&mm, 0x200000, 0x200000);
	// 0x....ffffc00.....
	pde[0].set(reinterpret_cast<_u64>(p1),
		arch::pte::P | arch::pte::RW | arch::pte::PS | arch::pte::G);

	if (kern_extract(&mm) == false) {
		return -1;
	}

	// Kernel stack memory

	char* p2 = (char*)memmgr_alloc(&mm, 0x200000, 0x200000);
	// 0x....ffffffe.....
	pde[255].set(reinterpret_cast<_u64>(p2),
		arch::pte::P | arch::pte::RW | arch::pte::PS | arch::pte::G);

	pde_adr += 8 * 512;

	asm ("invlpg " TOSTR(KERN_FINAL_VADR));

	return 0;
}

