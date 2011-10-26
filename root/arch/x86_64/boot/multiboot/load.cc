/// @file   load.cc
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"
#include "misc.hh"
#include "pagetable.hh"
#include "string.hh"
#include "vga.hh"

#include "elf.hh"


extern u8 core[];
extern u8 core_size[];

namespace {

/// @brief  オブジェクトをページへコピーする。
/// @param[in] phe  ELF program header.
/// @param[in] page_vadr  phys_vadr のマッピング先ページアドレス
/// @param[in] page_size  page_vadr と phys_vadr のページサイズ
/// @param[in] phys_vadr  実際にコピーするアドレス。
//
/// phys_vadr が page_vadr へマッピングされる前提で、phys_vadr へ
/// オブジェクトをコピーする。
/// コピー元は page_vadr から計算する。
cause::stype load_segm_page(
    const Elf64_Phdr* phe,
    u64               page_vadr,  ///< マップ先ページアドレス
    uptr              page_size,  ///< マップ先ページサイズ
    uptr              phys_vadr)  ///< コピー先アドレス
{
	u64 map_vadr = page_vadr;
	uptr dest_size = page_size;
	u8* dest = reinterpret_cast<u8*>(phys_vadr);

	// map_vadr が phe->p_vaddr より前を指しているときは、
	// そのギャップを 0xAA で埋める。
	if (map_vadr < phe->p_vaddr) {
		const uptr gap_size = phe->p_vaddr - map_vadr;
		memory_fill(0xAA, dest, gap_size);
log()("fill adr = ").u(phys_vadr, 16)(", gap_size = ").u(gap_size, 16)();

		map_vadr += gap_size;
		dest += gap_size;
		dest_size -= gap_size;
	}

	// オブジェクトをコピーする。
	const u64 file_offset = phe->p_offset + (map_vadr - phe->p_vaddr);
	const sptr copy_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_filesz, dest_size);
log()("copy(")(dest)(", ")(core + file_offset)(", ").s(copy_size, 16)(")")();
	mem_copy(dest, core + file_offset, copy_size);

	map_vadr += copy_size;
	dest += copy_size;
	dest_size -= copy_size;

	// BSS 領域を 0x00 で埋める。
	const sptr fill_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_memsz, dest_size);
log()("fill(")(dest)(", ").s(fill_size, 16)(")")();
	memory_fill(0x00, dest, fill_size);

	dest += fill_size;
	dest_size -= fill_size;

	// ページの残りを 0xAA で埋める。
log()("aa(")(dest)(", ").u(dest_size, 16)(")")();
	memory_fill(0xAA, dest, dest_size);

	return cause::OK;
}

cause::stype load_segm(const Elf64_Phdr* phe, arch::page_table* pg_tbl)
{
	log()("(load)");

	cause::stype r;

	u64 page_flags = arch::page_table::EXIST;
	if (phe->p_flags & PF_W)
		page_flags |= arch::page_table::WRITE;

	const u64 start_page = down_align<u64>(
	    phe->p_vaddr, arch::page::PHYS_L2_SIZE);
	const u64 end_page = down_align<u64>(
	    phe->p_vaddr + phe->p_memsz, arch::page::PHYS_L2_SIZE);

	for (u64 page_adr = start_page; ; page_adr += arch::page::PHYS_L2_SIZE)
	{
		uptr phys_adr;
		r = arch::page::alloc(arch::page::PHYS_L2, &phys_adr);
		if (r != cause::OK)
			return r;

		r = load_segm_page(
		    phe, page_adr, arch::page::PHYS_L2_SIZE, phys_adr);
		if (r != cause::OK)
			return r;

		pg_tbl->set_page(
		    page_adr, static_cast<u64>(phys_adr),
		    arch::page::PHYS_L2, page_flags);

		// end_page がメモリ空間の最後のページを指している場合は、
		// page_adr をインクリメントするとオーバーフローしてしまう。
		if (page_adr >= end_page)
			break;
	}

	return cause::OK;
}

}

extern "C" void load()
{
	log()("core : ")(core)(", core_size : ")(core_size)();

	mem_alloc(0x1000, 0x1000);
	mem_alloc(0x1000, 0x1000);
	void* p = mem_alloc(0x200000, 0x200000);
	log()("p1 = ")(p)();

	Elf64_Ehdr* elf = (Elf64_Ehdr*)core;
/*
	log()("e_ident : ").
		u((u8)elf->e_ident[0], 16)(" ").
		u((u8)elf->e_ident[1], 16)(" ").
		u((u8)elf->e_ident[2], 16)(" ").
		u((u8)elf->e_ident[3], 16)(" ").
		u((u8)elf->e_ident[4], 16)(" ").
		u((u8)elf->e_ident[5], 16)(" ").
		u((u8)elf->e_ident[6], 16)(" ").
		u((u8)elf->e_ident[7], 16)(" ")();
	log()("e_type : ").u((u16)elf->e_type, 16)
		(", e_machine : ").u((u16)elf->e_machine)
		(", e_version : ").u((u32)elf->e_version)();
	log()("e_entry : ").u((u64)elf->e_entry, 16)();
	log()("e_phoff : ").u((u64)elf->e_phoff, 16)
		(", e_ehsize : ").u((u16)elf->e_ehsize)();
	log()("e_shoff : ").u((u64)elf->e_shoff)();
	log()("e_flags : ").u((u32)elf->e_flags)();
	log()("e_phentsize : ").u((u16)elf->e_phentsize)
		(", e_phnum : ").u((u16)elf->e_phnum)();
	log()("e_shentsize : ").u((u16)elf->e_shentsize)
		(", e_shnum : ").u((u16)elf->e_shnum)();
	log()("e_shstrndx : ").u((u16)elf->e_shstrndx)();
*/

	arch::page_table pg_tbl;
	u8* ph = core + elf->e_phoff;
	for (int i = 0; i < elf->e_phnum; ++i) {
		Elf64_Phdr* phe = (Elf64_Phdr*)ph;
		if (phe->p_type == PT_LOAD) {
			load_segm(phe, &pg_tbl);
		}
//		log()("p_type : ").u((u32)phe->p_type, 16)
//			(", p_flags : ").u((u32)phe->p_flags, 16)();
		log()("p_offset : ").u((u64)phe->p_offset, 16)
			(", p_align : ").u((u64)phe->p_align)();
		log()("p_vaddr : ").u((u64)phe->p_vaddr, 16)
			(", p_paddr : ").u((u64)phe->p_paddr, 16)();
		log()("p_filesz : ").u((u64)phe->p_filesz, 16)
			(", p_memsz : ").u((u64)phe->p_memsz, 16)();
		ph += elf->e_phentsize;
	}

	pg_tbl.dump(log());
}

extern "C" void post_load()
{
	extern u32 stack_start[];
	u32 i;
	for (i = 0; stack_start[i] == 0 && i < 0xffff; ++i);

	log()("left stack : ").u((i - 1) * 4);
}

