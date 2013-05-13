/// @file   load.cc
/// @brief  ELF kernel loader.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "misc.hh"

#include <bootinfo.hh>
#include <config.h>
#include <elf.hh>
#include <pagetable.hh>
#include <string.hh>


extern const u8 kernel[];
extern const u8 kernel_size[];

namespace {

class page_table_alloc
{
public:
	static cause::t alloc(uptr* padr);
	static cause::t free(uptr padr);
};

inline cause::t page_table_alloc::alloc(uptr* padr)
{
	void* p = get_alloc()->alloc(
	    SLOTM_BOOTHEAP,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);

	*padr = reinterpret_cast<uptr>(p);

	return p ? cause::OK : cause::NOMEM;
}

inline cause::t page_table_alloc::free(uptr padr)
{
	bool b = get_alloc()->dealloc(
	    SLOTM_BOOTHEAP, reinterpret_cast<void*>(padr));

	return b ? cause::OK : cause::FAIL;
}

inline void* phys_to_virt(u64 padr) {
	return reinterpret_cast<void*>(padr);
}

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
cause::t load_segm_page(
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

		map_vadr += gap_size;
		dest += gap_size;
		dest_size -= gap_size;
	}

	// オブジェクトをコピーする。
	const u64 file_offset = phe->p_offset + (map_vadr - phe->p_vaddr);
	const sptr copy_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_filesz, dest_size);
	mem_copy(copy_size, kernel + file_offset, dest);

	map_vadr += copy_size;
	dest += copy_size;
	dest_size -= copy_size;

	// BSS 領域を 0x00 で埋める。
	const sptr fill_size = min<sptr>(
	    (phe->p_vaddr - map_vadr) + phe->p_memsz, dest_size);
	mem_fill(fill_size, 0x00, dest);

	dest += fill_size;
	dest_size -= fill_size;

	// ページの残りを 0xAA で埋める。
	mem_fill(dest_size, 0xAA, dest);

	return cause::OK;
}

cause::t load_segm(const Elf64_Phdr* phe, boot_page_table* pg_tbl)
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
		cause::t r;

		uptr phys_adr;
		void* p = alloc->alloc(
		    SLOTM_NORMAL | SLOTM_BOOTHEAP,
		    arch::page::PHYS_L2_SIZE,
		    arch::page::PHYS_L2_SIZE,
		    false);
		if (!p)
			return cause::NOMEM;

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
	cause::t r = pre_load(magic, tag);
	if (r != cause::OK)
		return r;

	log(1)("kernel : ")(kernel)(", kernel_size : ")(kernel_size)();

	const Elf64_Ehdr* elf = reinterpret_cast<const Elf64_Ehdr*>(kernel);

#if CONFIG_DEBUG_BOOT >= 1
	log()("kernel ELF header:")()
	     ("e_ident : ").x(8, elf->e_ident, 1, 16)
	     (", e_type : ").x((u16)elf->e_type)
	     (", e_machine : ").u(elf->e_machine)
	     (", e_version : ").u(elf->e_version)()
	     ("e_entry : ").x((u64)elf->e_entry)
	     (", e_phoff : ").x((u64)elf->e_phoff)
	     (", e_ehsize : ").u(elf->e_ehsize)()
	     ("e_shoff : ").x(elf->e_shoff)
	     (", e_flags : ").u(elf->e_flags)()
	     ("e_phentsize : ").u(elf->e_phentsize)
	     (", e_phnum : ").u(elf->e_phnum)()
	     ("e_shentsize : ").u(elf->e_shentsize)
	     (", e_shnum : ").u(elf->e_shnum)()
	     ("e_shstrndx : ").u(elf->e_shstrndx)();
#endif  // CONFIG_DEBUG_BOOT >= 1

	boot_page_table pg_tbl(0, 0);

	const u8* ph = kernel + elf->e_phoff;
	for (int i = 0; i < elf->e_phnum; ++i) {
		const Elf64_Phdr* phe = reinterpret_cast<const Elf64_Phdr*>(ph);
		if (phe->p_type == PT_LOAD) {
			cause::t r = load_segm(phe, &pg_tbl);
			if (is_fail(r))
				return r;
		}
#if CONFIG_DEBUG_BOOT >= 1
		log()("program header[").u(i)("]:")()
		     (" p_type : ").x((u32)phe->p_type)
		     (", p_flags : ").x((u32)phe->p_flags)()
		     (" p_offset : ").x((u64)phe->p_offset)
		     (", p_align : ").u((u64)phe->p_align)()
		     (" p_vaddr : ").x((u64)phe->p_vaddr)
		     (", p_paddr : ").x((u64)phe->p_paddr)()
		     (" p_filesz : ").x((u64)phe->p_filesz)
		     (", p_memsz : ").x((u64)phe->p_memsz)();
#endif  // CONFIG_DEBUG_BOOT
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
	cause::type r = arch::page::alloc(arch::page::PHYS_L2, &stack_padr);
	if (r != cause::OK)
		return r;
	pg_tbl.set_page(0 - arch::page::PHYS_L2_SIZE, stack_padr,
	    arch::page::PHYS_L2,
	    arch::page_table::EXIST | arch::page_table::WRITE);
*/

	load_info.entry_adr = elf->e_entry;
	load_info.page_table_adr = reinterpret_cast<uptr>(pg_tbl.get_table());

	return post_load(tag);
}

