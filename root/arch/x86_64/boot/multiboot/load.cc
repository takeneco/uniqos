/// @file   load.cc
/// @brief  ELF kernel loader.
//
// (C) 2011 KATO Takeshi
//

#include "bootinfo.hh"
#include "elf.hh"
#include "log.hh"
#include "misc.hh"
#include "pagetable.hh"
#include "string.hh"


extern const u8 kernel[];
extern const u8 kernel_size[];

namespace {

class page_table_alloc
{
public:
	static cause::stype alloc(uptr* padr);
	static cause::stype free(uptr padr);
};

inline cause::stype page_table_alloc::alloc(uptr* padr)
{
	void* p = get_alloc()->alloc(
	    MEM_BOOTHEAP,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);

	*padr = reinterpret_cast<uptr>(p);

	return p ? cause::OK : cause::NO_MEMORY;
}

inline cause::stype page_table_alloc::free(uptr padr)
{
	bool b = get_alloc()->free(MEM_BOOTHEAP, reinterpret_cast<void*>(padr));

	return b ? cause::OK : cause::FAIL;
}

inline u64 phys_to_virt(u64 padr) { return padr; }

typedef arch::page_table<page_table_alloc, phys_to_virt> boot_page_table;


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
		mem_fill(gap_size, 0xAA, dest);
//log()("fill adr = ").u(phys_vadr, 16)(", gap_size = ").u(gap_size, 16)();

		map_vadr += gap_size;
		dest += gap_size;
		dest_size -= gap_size;
	}

	// オブジェクトをコピーする。
	const u64 file_offset = phe->p_offset + (map_vadr - phe->p_vaddr);
	const sptr copy_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_filesz, dest_size);
//log()("copy(")(dest)(", ")(kernel+file_offset)(", ").s(copy_size, 16)(")")();
	mem_copy(copy_size, kernel + file_offset, dest);

	map_vadr += copy_size;
	dest += copy_size;
	dest_size -= copy_size;

	// BSS 領域を 0x00 で埋める。
	const sptr fill_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_memsz, dest_size);
//log()("fill(")(dest)(", ").s(fill_size, 16)(")")();
	mem_fill(fill_size, 0x00, dest);

	dest += fill_size;
	dest_size -= fill_size;

	// ページの残りを 0xAA で埋める。
//log()("aa(")(dest)(", ").u(dest_size, 16)(")")();
	mem_fill(dest_size, 0xAA, dest);

	return cause::OK;
}

cause::stype load_segm(const Elf64_Phdr* phe, boot_page_table* pg_tbl)
{
	allocator* alloc = get_alloc();

	u64 page_flags = boot_page_table::EXIST;
	if (phe->p_flags & PF_W)
		page_flags |= boot_page_table::WRITE;

	const u64 start_page = down_align<u64>(
	    phe->p_vaddr, arch::page::PHYS_L2_SIZE);
	const u64 end_page = down_align<u64>(
	    phe->p_vaddr + phe->p_memsz, arch::page::PHYS_L2_SIZE);

	for (u64 page_adr = start_page; ; page_adr += arch::page::PHYS_L2_SIZE)
	{
		cause::stype r;

		uptr phys_adr;
		void* p = alloc->alloc(
		    MEM_NORMAL | MEM_BOOTHEAP,
		    arch::page::PHYS_L2_SIZE,
		    arch::page::PHYS_L2_SIZE,
		    false);
		if (!p)
			return cause::NO_MEMORY;

		phys_adr = reinterpret_cast<uptr>(p);
		r = load_segm_page(
		    phe, page_adr, arch::page::PHYS_L2_SIZE, phys_adr);
		if (r != cause::OK)
			return r;

		r = pg_tbl->set_page(page_adr, static_cast<u64>(phys_adr),
		    arch::page::PHYS_L2, page_flags);
		if (r != cause::OK)
			return r;

		// end_page がメモリ空間の最後のページを指している場合は、
		// page_adr をインクリメントするとオーバーフローしてしまう。
		if (page_adr >= end_page)
			break;
	}

	return cause::OK;
}

}  // namespace

extern "C" u32 load(u32 magic, u32* tag)
{
	cause::stype r = pre_load(magic, tag);
	if (r != cause::OK)
		return r;

	log()("kernel : ")(kernel)(", kernel_size : ")(kernel_size)();

	const Elf64_Ehdr* elf = reinterpret_cast<const Elf64_Ehdr*>(kernel);
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
	boot_page_table pg_tbl(0, 0);

	const u8* ph = kernel + elf->e_phoff;
	for (int i = 0; i < elf->e_phnum; ++i) {
		const Elf64_Phdr* phe = reinterpret_cast<const Elf64_Phdr*>(ph);
		if (phe->p_type == PT_LOAD) {
			cause::stype r = load_segm(phe, &pg_tbl);
			if (is_fail(r))
				return r;
		}
//		log()("p_type : ").u((u32)phe->p_type, 16)
//			(", p_flags : ").u((u32)phe->p_flags, 16)();
//		log()("p_offset : ").u((u64)phe->p_offset, 16)
//			(", p_align : ").u((u64)phe->p_align)();
//		log()("p_vaddr : ").u((u64)phe->p_vaddr, 16)
//			(", p_paddr : ").u((u64)phe->p_paddr, 16)();
//		log()("p_filesz : ").u((u64)phe->p_filesz, 16)
//			(", p_memsz : ").u((u64)phe->p_memsz, 16)();
		ph += elf->e_phentsize;
	}

	// カーネルが自分でメモリ管理できるようになるまでの
	// 一時的なヒープとして使う。
	for (int adr = 0;
	     adr < bootinfo::BOOTHEAP_END;
	     adr += arch::page::PHYS_L2_SIZE)
	{
		r = pg_tbl.set_page(adr, adr,
		    arch::page::PHYS_L2, boot_page_table::EXIST);
		if (r != cause::OK)
			return r;
	}

	// ページテーブル自身をページテーブルにマップする。
	u64 pg_tbl_top = reinterpret_cast<u64>(pg_tbl.get_table());
	pg_tbl.set_page(pg_tbl_top, pg_tbl_top,
	    arch::page::PHYS_L1, boot_page_table::EXIST);
	if (r != cause::OK)
		return r;

/*	// sharing stack with .bss section
	// stack
	uptr stack_padr;
	cause::stype r = arch::page::alloc(arch::page::PHYS_L2, &stack_padr);
	if (r != cause::OK)
		return r;
	pg_tbl.set_page(0 - arch::page::PHYS_L2_SIZE, stack_padr,
	    arch::page::PHYS_L2,
	    arch::page_table::EXIST | arch::page_table::WRITE);
*/

	//pg_tbl.dump(log());

	load_info.entry_adr = elf->e_entry;
	load_info.page_table_adr = reinterpret_cast<uptr>(pg_tbl.get_table());

	return post_load(tag);
}

