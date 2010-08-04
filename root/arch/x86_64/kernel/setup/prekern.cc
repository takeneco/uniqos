// @file    arch/x86_64/kernel/setup/prekern.cc
// @author  Kato Takeshi
// @brief   Kernel extend phase before execute.
//
// (C) 2010 Kato Takeshi.

#include "access.hh"
#include "lzmadecwrap.hh"
#include "pagetable.hh"

#include "term.hh"
#include "native.hh"


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
	const u32 setup_size = &kern_body_start - &setup_body_start;

	u8* const comp_kern_src =
		reinterpret_cast<u8*>(SETUP_KERN_ADR + setup_size);

	const u32 comp_kern_size =
		setup_get_value<u32>(SETUP_KERNFILE_SIZE) - setup_size;

	const u32 ext_kern_size = lzma_decode_size(comp_kern_src);

	u8* const ext_kern_dest =
		reinterpret_cast<u8*>(KERN_FINAL_VADR);

	return lzma_decode(mm, comp_kern_src, comp_kern_size,
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

void fixed_fhysical_memmap_alloc(u64* pde_adr)
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

void fixed_allocs(memmgr* mm)
{
	page_table_ent* pdpte_base =
		reinterpret_cast<page_table_ent*>(KERN_PDPTE_PADR);
	u64 pde_adr = KERN_PDE_PADR;
	page_table_ent* pde;

// Physical address mapping start (for debug)
/*
	// pdpte_base[0] -> 0x....80000.......
	pdpte_base[0].set(pde_adr, page_table_ent::P | page_table_ent::RW);

	page_table_ent* pde = reinterpret_cast<arch::pte*>(pde_adr);
	for (int i = 0; i < 512; i++) { // 1GiB
		pde[i].set(0x200000 * i,
		    page_table_ent::P |
		    page_table_ent::RW |
		    page_table_ent::PS |
		    page_table_ent::G);
	}

	pde_adr += 8 * 512;
*/
// Physical address mapping end


// Special address mapping start

	// pdpte_base[0x1fff8] -> 0x....fffe0.......
	pdpte_base[0x1fff8].set(pde_adr, page_table_ent::P | page_table_ent::RW);

	pde = reinterpret_cast<arch::pte*>(pde_adr);

	// ����٥󥷥�ʥ�����ޤ���Ƭ 20MiB (2MiB * 10)
	for (int i = 0; i < 10; i++) {
		pde[i].set(0x200000 * i,
		    page_table_ent::P |
		    page_table_ent::RW |
		    page_table_ent::PS |
		    page_table_ent::G);
	}

	// APIC register
	pde[10].set(0xfee00000,
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	for (int i = 11; i < 512; i++) {
		pde[i].set(0, 0);
	}

	pde_adr += 8 * 512;

// Special address mapping end


// Kernel text body address mapping start

	// pdpte_base[0x1fffc] -> 0x....ffff0.......
	pdpte_base[0x1fffc].set(pde_adr, page_table_ent::P | page_table_ent::RW);

	pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p1 = (char*)memmgr_alloc(mm, 0x200000, 0x200000);
	// 0x....ffffc00.....
	pde[0].set(reinterpret_cast<u64>(p1),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	pde_adr += 8 * 512;

// Kernel text body address mapping end


	// pdpte_base[0x1ffff] -> 0x....ffffc.......
	pdpte_base[0x1ffff].set(pde_adr, page_table_ent::P | page_table_ent::RW);

	// Kernel stack memory

	pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p2 = (char*)memmgr_alloc(mm, 0x200000, 0x200000);
	// 0x....ffffffe.....
	pde[511].set(reinterpret_cast<u64>(p2),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	// test map
	// 0x....ffffc02.....
	pde[1].set(0,
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	pde_adr += 8 * 512;
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
		setup_get_value<u32>(SETUP_DISP_VRAM));
		//setup_get_value<u32>(SETUP_DISP_VRAM) +
		//0xffff800000000000L);
	vt.set_cur(
		setup_get_value<u32>(SETUP_DISP_CURROW),
		setup_get_value<u32>(SETUP_DISP_CURCOL));

	term_chain tc;
	tc.add_term(&vt);

	debug_tc = &tc;

	tc.puts("DISPLAY : ")
	->putu64(setup_get_value<u32>(SETUP_DISP_WIDTH))
	->putc('x')
	->putu64(setup_get_value<u32>(SETUP_DISP_HEIGHT))
	->putc('\n');

	tc.puts("Memorymap by ACPI : \n");

	const acpi_memmap* memmap_buf =
		setup_get_ptr<const acpi_memmap>(SETUP_MEMMAP);
	const int memmaps = setup_get_value<u32>(SETUP_MEMMAP_COUNT);
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

	memmgr mm;
	memmgr_init(&mm);

	// �ǽ����Ū�˻��Ѥ������� fixed_allocs() ����Ƭ���������Ƥ롣
	// ���θ�� mm ��ưŪ����γ��ݤ˻��Ѥ��롣ưŪ����ϳ������Ƥ���
	// kernel body �˥����פ��롣

	fixed_allocs(&mm);

	if (kern_extract(&mm) == false) {
		return -1;
	}

	asm ("invlpg " TOSTR(KERN_FINAL_VADR));

	int currow, curcol;
	vt.get_cur(&currow, &curcol);
	setup_set_value<u32>(SETUP_DISP_CURROW, currow);
	setup_set_value<u32>(SETUP_DISP_CURCOL, curcol);

	memmgr_dump(&mm, setup_get_ptr<memmgr_dumpdata>(SETUP_MEMMAP_DUMP), 32);

	return 0;
}

