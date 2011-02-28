/// @file  prekernel.cc
/// @brief Kernel extend phase before execute.
//
// (C) 2010 KATO Takeshi
//

#include "access.hh"
#include "misc.hh"
#include "pagetable.hh"
#include "arch.hh"

#include "term.hh"
#include "native_ops.hh"


extern char setup_body_start;
extern char kern_body_start;

term_chain* debug_tc;

namespace {

const char* acpi_memtype(int type)
{
	const static char* type_name[] = {
		"unknown", "memory", "reserved",
		"acpi", "nvs", "unusuable" };

	if (1 <= type && type <= 5)
		return type_name[type];
	else
		return type_name[0];
}

bool kern_extract()
{
	const u32 setup_size = &kern_body_start - &setup_body_start;

	u8* const comp_kern_src =
		reinterpret_cast<u8*>(SETUP_KERN_ADR + setup_size);

	const u32 comp_kern_size =
		setup_get_value<u32>(SETUP_KERNFILE_SIZE) - setup_size;

	const u32 ext_kern_size = lzma_decode_size(comp_kern_src);

	u8* const ext_kern_dest =
		reinterpret_cast<u8*>(KERN_FINAL_VADR);

	debug_tc->puts("&kern_body_start = ")->putu64x((u64)&kern_body_start)->putc('\n');
	debug_tc->puts("&setup_body_start = ")->putu64x((u64)&setup_body_start)->putc('\n');
	debug_tc->puts("setup_size = ")->putu64(setup_size)->putc('\n');
	debug_tc->puts("comp_kern_src = ")->putu64x((u64)comp_kern_src)->putc('\n');
	debug_tc->puts("comp_kern_size = ")->putu64(comp_kern_size)->putc('\n');
	debug_tc->puts("ext_kern_size = ")->putu64(ext_kern_size)->putc('\n');
	debug_tc->puts("ext_kern_dest = ")->putu64x((u64)ext_kern_dest)->putc('\n');

	return lzma_decode(comp_kern_src, comp_kern_size,
		ext_kern_dest, ext_kern_size);
}

/// �����ͥ������֤�PDPTE���֤���
//  �����ͥ������֤�PDPTE��Ϣ³�������˳�Ǽ����Ƥ��롣
inline page_table_ent* pdpte_base_addr()
{
	// pdpte_base[512 *   0] -> 0x....800.........
	// pdpte_base[512 *   1] -> 0x....808.........
	// pdpte_base[512 * 254] -> 0x....ff0.........
	// pdpte_base[512 * 255] -> 0x....ff8.........
	return reinterpret_cast<page_table_ent*>(KERN_PDPTE_PADR);
}
/*
// not used
void fixed_physical_memmap_alloc(u64* pde_adr)
{
	page_table_ent* pdpte_base = pdpte_base_addr();

	// pdpte_base[0] -> 0x....80000.......
	for (int i = 0; i < 4; i++) {  // 4GiB
		pdpte_base[i].set(*pde_adr,
		    page_table_ent::P | page_table_ent::RW);

		page_table_ent* pde = reinterpret_cast<page_table_ent*>(*pde_adr);
		for (int j = 0; j < 512; j++) {
			pde[j].set(0x200000 * j,
			    page_table_ent::P |
			    page_table_ent::RW |
			    page_table_ent::PS |
			    page_table_ent::G);
		}
		*pde_adr += 8 * 512;
	}
}
*/
void fixed_allocs()
{
	page_table_ent* pdpte_base =
		reinterpret_cast<page_table_ent*>(KERN_PDPTE_PADR);
	//u64 pde_adr = KERN_PDE_PADR;

// Kernel text body address mapping start

	page_table_ent* pde = reinterpret_cast<page_table_ent*>
	    (memmgr_alloc(arch::PAGE_L1_SIZE, arch::PAGE_L1_SIZE));

	// pdpte_base[0x1fffc] -> 0x....ffff0.......
	pdpte_base[0x1fffc].set(reinterpret_cast<u64>(pde) /*pde_adr*/,
	    page_table_ent::P | page_table_ent::RW);

	//page_table_ent* pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p1 = (char*)memmgr_alloc(0x200000, 0x200000);
	// 0x....ffffc00.....
	pde[0].set(reinterpret_cast<u64>(p1),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	//pde_adr += 8 * 512;

// Kernel text body address mapping end

	pde = reinterpret_cast<page_table_ent*>
	    (memmgr_alloc(arch::PAGE_L1_SIZE, arch::PAGE_L1_SIZE));

	// pdpte_base[0x1ffff] -> 0x....ffffc.......
	pdpte_base[0x1ffff].set(reinterpret_cast<u64>(pde) /*pde_adr*/,
	    page_table_ent::P | page_table_ent::RW);

	// Kernel stack memory

	//pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p2 = (char*)memmgr_alloc(0x200000, 0x200000);
	// 0x....ffffffe.....
	pde[511].set(reinterpret_cast<u64>(p2),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	//pde_adr += 8 * 512;
}

}  // End of anonymous namespace

void intr_init();

/**
 * @brief  Create memory map, Kernel body extract, etc...
 * @retval 0     Succeeds. Kernel body exectable.
 * @retval other Fails.
 */
extern "C" int prekernel()
{
	video_term vt;
	vt.init(
		setup_get_value<u32>(SETUP_DISP_WIDTH),
		setup_get_value<u32>(SETUP_DISP_HEIGHT),
		setup_get_value<u32>(SETUP_DISP_VRAM) +
		0xffff800000000000L);
	vt.set_cur(
		setup_get_value<u32>(SETUP_DISP_CURROW),
		setup_get_value<u32>(SETUP_DISP_CURCOL));

	term_chain tc;
	tc.add_term(&vt);

	debug_tc = &tc;

	tc.puts("Memorymap by ACPI : \n");

	const acpi_memmap* memmap_buf =
		setup_get_ptr<const acpi_memmap>(SETUP_ACPI_MEMMAP);
	const int memmaps = setup_get_value<u32>(SETUP_ACPI_MEMMAP_COUNT);
	if (memmaps < 0) {
		return -1;
	}
	for (int i = 0; i < memmaps; i++) {
		const u64 base = memmap_buf[i].base;
		const u64 length = memmap_buf[i].length;
		const u32 type = memmap_buf[i].type;
		tc.putu64(i)
		->puts(" : ")
		->putu64x(base)
		->puts(" - ")
		->putu64x(base + length)
		->putc(' ')
		->puts(acpi_memtype(type))
		->putc('\n');
	}

	memmgr_init();

	// �ǽ����Ū�˻��Ѥ������� fixed_allocs() ����Ƭ���������Ƥ롣
	// ���θ�� mm ��ưŪ����γ��ݤ˻��Ѥ��롣ưŪ����ϳ������Ƥ���
	// kernel body �˥����פ��롣

	fixed_allocs();

	if (kern_extract() == false)
	{
		tc.puts("x");
		return -1;
	}

	asm ("invlpg " TOSTR(KERN_FINAL_VADR));

	int currow, curcol;
	vt.get_cur(&currow, &curcol);
	setup_set_value<u32>(SETUP_DISP_CURROW, currow);
	setup_set_value<u32>(SETUP_DISP_CURCOL, curcol);

	int dumps = memmgr_freemem_dump(
	    setup_get_ptr<setup_memmgr_dumpdata>(SETUP_FREEMEM_DUMP), 32);
	setup_set_value<u32>(SETUP_FREEMEM_DUMP_COUNT, dumps);

	dumps = memmgr_nofreemem_dump(
	    setup_get_ptr<setup_memmgr_dumpdata>(SETUP_USEDMEM_DUMP), 32);
	setup_set_value<u32>(SETUP_USEDMEM_DUMP_COUNT, dumps);

	return 0;
}

