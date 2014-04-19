/// @file   preload_mb2.cc
/// @brief  Prepare to load kernel for multiboot2.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "misc.hh"

#include <bootinfo.hh>
#include <config.h>
// multiboot と multiboot2 はインクルードガードがかぶっている
#include <multiboot2.h>
#include <vga.hh>


// このコード自身のメモリ上のアドレスとサイズ。
extern "C" u8 self_baseadr[];
extern "C" u8 self_size[];

extern text_vga vga_dev;

namespace {

void mem_setup(const multiboot_tag_mmap* mbt_mmap, const u32* tag)
{
//	log()("memmap : esize=").u(u32(mbt_mmap->entry_size))
//	    (", eversion=").u(u32(mbt_mmap->entry_version))();

	init_alloc();

	allocator* alloc = get_alloc();
	separator sep(alloc);
	init_separator(&sep);

	const void* end = (const u8*)mbt_mmap + mbt_mmap->size;
	const multiboot_memory_map_t* mmap = mbt_mmap->entries;
	while (mmap < end) {
		/*
		log()(" ").u(u64(mmap->addr), 16)
		    (" len=").u(u64(mmap->len), 16)
		    (" type=").u(u32(mmap->type))();
		*/

		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
			const uptr MEMMAX = 0xfffff000;
			uptr addr, len;

			if (mmap->addr <= MEMMAX)
				addr = mmap->addr;
			else
				continue;

			if ((mmap->addr + mmap->len) <= MEMMAX)
				len = mmap->len;
			else if (mmap->addr <= MEMMAX)
				len = MEMMAX - mmap->addr;
			else
				continue;

			sep.add_free(addr, len);
		}

		mmap = (const multiboot_memory_map_t*)
		    ((const u8*)mmap + mbt_mmap->entry_size);
	}

	// self memory
	alloc->reserve(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP,
	    reinterpret_cast<uptr>(self_baseadr),
	    reinterpret_cast<uptr>(self_size),
	    true);

	// tag
	const u32 tag_size = *tag;
	alloc->reserve(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL,
	    reinterpret_cast<uptr>(tag),
	    tag_size,
	    true);

	// BDA : BIOS Data Area 0 -  0x4ff
	// Startup IPI use        - 0x1fff
	alloc->reserve(SLOTM_CONVENTIONAL, 0, 0x2000, false);
}

cause::t mbmmap_to_adrmap(
    const multiboot_memory_map_t* mmap_ent,
    bootinfo::adr_map::entry*     adrmap_ent)
{
	adrmap_ent->adr = mmap_ent->addr;

	adrmap_ent->bytes = mmap_ent->len;

	switch (mmap_ent->type) {
	case MULTIBOOT_MEMORY_AVAILABLE:
		adrmap_ent->type = bootinfo::adr_map::entry::AVAILABLE;
		break;
	case  MULTIBOOT_MEMORY_RESERVED:
		adrmap_ent->type = bootinfo::adr_map::entry::RESERVED;
		break;
	case  MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
		adrmap_ent->type = bootinfo::adr_map::entry::ACPI;
		break;
	case  MULTIBOOT_MEMORY_NVS:
		adrmap_ent->type = bootinfo::adr_map::entry::NVS;
		break;
	case  MULTIBOOT_MEMORY_BADRAM:
		adrmap_ent->type = bootinfo::adr_map::entry::BADRAM;
		break;
	default:
		return cause::BADARG;
	}

	adrmap_ent->zero = 0;

	return cause::OK;
}

/// @brief multiboot_mmap_entry を bootinfo::adrmap に変換する
cause::t store_adrmap(const multiboot_tag_mmap* mbt_mmap)
{
	allocator* alloc = get_alloc();

	int ents = (mbt_mmap->size - sizeof *mbt_mmap) / mbt_mmap->entry_size;

	uptr adr_map_bytes = sizeof (bootinfo::adr_map) +
		             sizeof (bootinfo::adr_map::entry) * ents;

	bootinfo::adr_map* adrmap = static_cast<bootinfo::adr_map*>(
	    alloc->alloc(
	        SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL,
	        adr_map_bytes,
	        4096,
	        true));

	const void* mmap_end = (const u8*)mbt_mmap + mbt_mmap->size;
	const multiboot_memory_map_t* mmap = mbt_mmap->entries;
	int adrmap_i = 0;
	while (mmap < mmap_end) {
		auto r = mbmmap_to_adrmap(mmap, &adrmap->entries[adrmap_i]);
		if (is_fail(r))
			return r;

		mmap = (const multiboot_memory_map_t*)
		    ((const u8*)mmap + mbt_mmap->entry_size);
		++adrmap_i;
	}

	adrmap->info_type = bootinfo::TYPE_ADR_MAP;
	adrmap->info_flags = 0;
	adrmap->info_bytes = adr_map_bytes;

	adr_map_store = adrmap;

	return cause::OK;
}

}  // namespace


/// @brief  Prepare to load kernel.
/// @param[in] magic  multiboot magic code.
/// @param[in] tag    multiboot info.
cause::t pre_load_mb2(u32 magic, const u32* tag)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		return cause::BADARG;

	cause::t r = memlog_file::setup();
	if (is_fail(r))
		return r;

	// temporary
	vga_dev.init(80, 25, (void*)0xb8000);

	// memlog を設定しているが、memlog.open() するまで memlog への
	// ログ出力はできない。
	// memlog.open() の前に mem_setup() する必要がある。
	log_set(0, &memlog);
	log_set(1, &vga_dev);

	const u32* p = tag;
	const u32 tag_size = *p;
	p += 2;

	u32 read = 8;
	while (read < tag_size) {
		const multiboot_tag* mbt =
		    reinterpret_cast<const multiboot_tag*>(p);
		switch (mbt->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE: {
			/*
			const multiboot_tag_string* mbt_cmdline =
			    reinterpret_cast<const multiboot_tag_string*>(mbt);
			log(1)("cmdline : [")(mbt_cmdline->string)("]")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
			/*
			const multiboot_tag_string* mbt_bootldr =
			    reinterpret_cast<const multiboot_tag_string*>(mbt);
			log(1)("bootldr : [")(mbt_bootldr->string)("]")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_MODULE:
			if (CONFIG_DEBUG_BOOT >= 1)
				log(1)("modules tag availavle.")();
			break;

		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
			/*
			const multiboot_tag_basic_meminfo* mbt_bmem =
			    reinterpret_cast<const multiboot_tag_basic_meminfo*>
			    (mbt);
			log(1)("basic memory : lower=").
			    u(u32(mbt_bmem->mem_lower))("KB, upper=").
			    u(u32(mbt_bmem->mem_upper))("KB")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOTDEV: {
			/*
			const multiboot_tag_bootdev* mbt_bootdev =
			    reinterpret_cast<const multiboot_tag_bootdev*>(mbt);
			log(1)("bios boot device : ").
			    u(u32(mbt_bootdev->biosdev), 16)(", ").
			    u(u32(mbt_bootdev->slice), 16)(", ").
			    u(u32(mbt_bootdev->part), 16)();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_MMAP: {
			const multiboot_tag_mmap* mbt_mmap =
			    reinterpret_cast<const multiboot_tag_mmap*>(mbt);
			mem_setup(mbt_mmap, tag);

			auto r = store_adrmap(mbt_mmap);
			if (is_fail(r))
				return r;
			break;
		}
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS: {
			/*
			const multiboot_tag_elf_sections* mbt_elfsec =
			    (const multiboot_tag_elf_sections*)mbt;
			log(1)("elf section : num=").u(u32(mbt_elfsec->num))
			    (", entsize=").u(u32(mbt_elfsec->entsize))
			    (", shndx=").u(u32(mbt_elfsec->shndx))();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
			const multiboot_tag_framebuffer* mbt_fb =
			    reinterpret_cast<const multiboot_tag_framebuffer*>
			    (mbt);
			/*
			log(1)("framebuffer : addr=")
			    .u(mbt_fb->common.framebuffer_addr, 16)
			    (", width=").u(mbt_fb->common.framebuffer_width)
			    (", height=").u(mbt_fb->common.framebuffer_height)
			    (", type=").u(mbt_fb->common.framebuffer_type);
			*/
			if (mbt_fb->common.framebuffer_type ==
			    MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT)
			{
				vga_dev.init(
				    mbt_fb->common.framebuffer_width,
				    mbt_fb->common.framebuffer_height,
				    (void*)mbt_fb->common.framebuffer_addr);
			}
			break;
		}
		case MULTIBOOT_TAG_TYPE_END:
			read = tag_size; // force loop break
			break;

		default:
			/*
			log(1)("multiboot2 unknown type info : ")
			    .u(u32(mbt->type))();
			*/
			break;
		}

		const u32 dec = up_align<u32>(mbt->size, MULTIBOOT_TAG_ALIGN);
		p += dec / sizeof *p;
		read += dec;
	}

	r = memlog.open();
	if (is_fail(r))
		return r;

	return cause::OK;
}

