/// @file   preload.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2013 KATO Takeshi
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
#include <multiboot2.h>
#include <vga.hh>


extern "C" u8 self_baseadr[];
extern "C" u8 self_size[];

namespace {

text_vga vga_dev;

const uptr BOOTHEAP_END = bootinfo::BOOTHEAP_END;

const struct {
	allocator::slot_index slot;
	uptr slot_head;
	uptr slot_tail;
} memory_slots[] = {
	{ SLOT_INDEX_CONVENTIONAL, 0x00000000,       0x000fffff   },
	{ SLOT_INDEX_BOOTHEAP,     0x00100000,       BOOTHEAP_END },
	{ SLOT_INDEX_NORMAL,       BOOTHEAP_END + 1, 0xffffffff   },
};

void init_sep(separator* sep)
{
	for (u32 i = 0; i < sizeof memory_slots / sizeof memory_slots[0]; ++i)
	{
		sep->set_slot_range(
		    memory_slots[i].slot,
		    memory_slots[i].slot_head,
		    memory_slots[i].slot_tail);
	}
}

void mem_setup(const multiboot_tag_mmap* mbt_mmap, const u32* tag)
{
//	log()("memmap : esize=").u(u32(mbt_mmap->entry_size))
//	    (", eversion=").u(u32(mbt_mmap->entry_version))();

	init_alloc();

	allocator* alloc = get_alloc();
	separator sep(alloc);
	init_sep(&sep);

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

}  // namespace


/// @brief  Previous load kernel.
/// @param[in] magic  multiboot magic code.
/// @param[in] tag    multiboot info.
cause::t pre_load_mb2(u32 magic, const u32* tag)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		return cause::BADARG;

	cause::stype r = memlog_file::setup();
	if (is_fail(r))
		return r;

	// temporary
	vga_dev.init(80, 25, (void*)0xb8000);

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
			log()("cmdline : [")(mbt_cmdline->string)("]")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
			/*
			const multiboot_tag_string* mbt_bootldr =
			    reinterpret_cast<const multiboot_tag_string*>(mbt);
			log()("bootldr : [")(mbt_bootldr->string)("]")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_MODULE:
			if (CONFIG_DEBUG_BOOT >= 1)
				log()("modules tag availavle.")();
			break;

		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
			/*
			const multiboot_tag_basic_meminfo* mbt_bmem =
			    reinterpret_cast<const multiboot_tag_basic_meminfo*>
			    (mbt);
			log()("basic memory : lower=").
			    u(u32(mbt_bmem->mem_lower))("KB, upper=").
			    u(u32(mbt_bmem->mem_upper))("KB")();
			*/
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOTDEV: {
			/*
			const multiboot_tag_bootdev* mbt_bootdev =
			    reinterpret_cast<const multiboot_tag_bootdev*>(mbt);
			log()("bios boot device : ").
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
			break;
		}
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS: {
			/*
			const multiboot_tag_elf_sections* mbt_elfsec =
			    (const multiboot_tag_elf_sections*)mbt;
			log()("elf section : num=").u(u32(mbt_elfsec->num))
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
			//log()("unknown type info : ").u(u32(mbt->type))();
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

