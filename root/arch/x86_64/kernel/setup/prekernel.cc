/// @file  prekernel.cc
/// @brief Kernel extend phase before execute.
//
// (C) 2010-2011 KATO Takeshi
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

u64 get_memory_end()
{
	const acpi_memmap* mm =
	    setup_get_ptr<const acpi_memmap>(SETUP_ACPI_MEMMAP);

	const u32 n = setup_get_value<u32>(SETUP_ACPI_MEMMAP_COUNT);

	u64 end = 0;
	for (u32 i = 0; i < n; ++i) {
		const u64 end_ = mm[i].base + mm[i].length;
		if (end < end_)
			end = end_;
	}

	return end;
}

void init_physical_memmap()
{
	const uptr end_padr = get_memory_end();
	const uptr pde_count = up_div<uptr>(end_padr, arch::PAGE_L2_SIZE);
	const uptr pde_table_count = up_div<uptr>(pde_count, 512);
	const uptr pde_table_size = pde_table_count * arch::PAGE_L1_SIZE;

	page_table_ent* pde = reinterpret_cast<page_table_ent*>(
	    memory_alloc(pde_table_size, arch::PAGE_L1_SIZE));

	if (!pde) {
		log()(__FILE__, __LINE__)("memory_alloc() failed.")();
		native::hlt();
	}

	// create PDE
	uptr i;
	for (i = 0; i < pde_count; ++i) {
		pde[i].set(i * arch::PAGE_L2_SIZE,
		    page_table_ent::P |
		    page_table_ent::RW |
		    page_table_ent::PS |
		    page_table_ent::G);
	}
	for (; i < pde_table_count * 512; ++i) {
		pde[i].set(0, 0);
	}

	// update PDPTE
	page_table_ent* pdpte =
	    reinterpret_cast<page_table_ent*>(KERN_PDPTE_PADR);

	for (i = 0; i < pde_table_count; ++i) {
		pdpte[i].set(reinterpret_cast<uptr>(&pde[i * 512]),
		    page_table_ent::P |
		    page_table_ent::RW |
		    page_table_ent::G);
	}

	log()("pde = ")(pde)();
	log()("pdpte = ")(pdpte)();
}

const char* acpi_memtype(int type)
{
	const static char* type_name[] = {
		"unknown", "memory", "reserved",
		"acpi", "nvs", "unusable" };

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

	log()("&kern_body_start = ").u((u64)&kern_body_start, 16)();
	log()("&setup_body_start = ").u((u64)&setup_body_start, 16)();
	log()("setup_size = ").u(setup_size, 16)();
	log()("comp_kern_src = ")(comp_kern_src)();
	log()("comp_kern_size = ").u(comp_kern_size, 16)();
	log()("ext_kern_size = ").u(ext_kern_size, 16)();
	log()("ext_kern_dest = ")(ext_kern_dest)();

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
	    (memory_alloc(arch::PAGE_L1_SIZE, arch::PAGE_L1_SIZE));

	// pdpte_base[0x1fffc] -> 0x....ffff0.......
	pdpte_base[0x1fffc].set(reinterpret_cast<u64>(pde) /*pde_adr*/,
	    page_table_ent::P | page_table_ent::RW);

	//page_table_ent* pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p1 = (char*)memory_alloc(0x200000, 0x200000);
	// 0x....ffffc00.....
	pde[0].set(reinterpret_cast<u64>(p1),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	//pde_adr += 8 * 512;

// Kernel text body address mapping end

	pde = reinterpret_cast<page_table_ent*>
	    (memory_alloc(arch::PAGE_L1_SIZE, arch::PAGE_L1_SIZE));

	// pdpte_base[0x1ffff] -> 0x....ffffc.......
	pdpte_base[0x1ffff].set(reinterpret_cast<u64>(pde) /*pde_adr*/,
	    page_table_ent::P | page_table_ent::RW);

	// Kernel stack memory

	//pde = reinterpret_cast<page_table_ent*>(pde_adr);
	char* p2 = (char*)memory_alloc(0x200000, 0x200000);
	// 0x....ffffffe.....
	pde[511].set(reinterpret_cast<u64>(p2),
	    page_table_ent::P |
	    page_table_ent::RW |
	    page_table_ent::PS |
	    page_table_ent::G);

	//pde_adr += 8 * 512;
}


}  // namespace

void intr_init();

/**
 * @brief  Create memory map, Kernel body extract, etc...
 * @retval 0     Succeeds. Kernel body exectable.
 * @retval other Fails.
 */
extern "C" int prekernel()
{
	on_memory_log oml;
	log_init(&oml);

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

	log()("Memorymap by ACPI :")();

	const acpi_memmap* memmap_buf =
		setup_get_ptr<const acpi_memmap>(SETUP_ACPI_MEMMAP);
	const s32 memmaps = setup_get_value<u32>(SETUP_ACPI_MEMMAP_COUNT);
	if (memmaps < 0) {
		return -1;
	}
	for (u32 i = 0; i < static_cast<u32>(memmaps); i++) {
		const u64 base = memmap_buf[i].base;
		const u64 length = memmap_buf[i].length;
		const u32 type = memmap_buf[i].type;
		log().u(i)(" : ")
		    .u(base, 16)(" - ")
		    .u(base + length, 16)(' ')
		    (acpi_memtype(type))();
	}

	memory_init();
	init_physical_memmap();

	// �ǽ����Ū�˻��Ѥ������� fixed_allocs() ����Ƭ���������Ƥ롣
	// ���θ�� mm ��ưŪ����γ��ݤ˻��Ѥ��롣ưŪ����ϳ������Ƥ���
	// kernel body �˥����פ��롣

	fixed_allocs();

	if (kern_extract() == false)
	{
		log()("x")();
		return -1;
	}

	asm ("invlpg " TOSTR(KERN_FINAL_VADR));

	int currow, curcol;
	vt.get_cur(&currow, &curcol);
	setup_set_value<u32>(SETUP_DISP_CURROW, currow);
	setup_set_value<u32>(SETUP_DISP_CURCOL, curcol);

	int dumps = freemem_dump(
	    setup_get_ptr<setup_memory_dumpdata>(SETUP_FREEMEM_DUMP), 32);
	setup_set_value<u32>(SETUP_FREEMEM_DUMP_COUNT, dumps);

	dumps = nofreemem_dump(
	    setup_get_ptr<setup_memory_dumpdata>(SETUP_USEDMEM_DUMP), 32);
	setup_set_value<u32>(SETUP_USEDMEM_DUMP_COUNT, dumps);

	hpet_init();

	return 0;
}

