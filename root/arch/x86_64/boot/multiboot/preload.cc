/// @file   preload.cc
//
// (C) 2011 KATO Takeshi
//

#include "bootinfo.hh"
#include "log.hh"
#include "misc.hh"
#include "multiboot2.h"
#include "vga.hh"


extern "C" u8 self_baseadr[];
extern "C" u8 self_size[];

text_vga vga_dev;
log_file vga_log;

namespace {

void init_sep(separator* sep)
{
	// 先頭から bootinfo::BOOTHEAP_END までをなるべく空けておく。
	sep->set_slot_range(
	    MEM_BOOTHEAP,
	    0,
	    bootinfo::BOOTHEAP_END);

	sep->set_slot_range(
	    MEM_NORMAL,
	    bootinfo::BOOTHEAP_END + 1,
	    0xffffffff);
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

		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
			sep.add_free(mmap->addr, mmap->len);

		mmap = (const multiboot_memory_map_t*)
		    ((const u8*)mmap + mbt_mmap->entry_size);
	}

	// self memory
	alloc->reserve(
	    MEM_NORMAL | MEM_BOOTHEAP,
	    reinterpret_cast<uptr>(self_baseadr),
	    reinterpret_cast<uptr>(self_size),
	    true);

	// tag
	const u32 tag_size = *tag;
	alloc->reserve(
	    MEM_NORMAL | MEM_BOOTHEAP,
	    reinterpret_cast<uptr>(tag),
	    tag_size,
	    true);

	// EBDA から bootinfo の領域をまとめて予約する。
	// EBDA:
	// mem_reserve(0, 0x4ff, false);
	// bootinfo:
	// mem_reserve(bootinfo::ADR, bootinfo::MAX_BYTES, false);
	alloc->reserve(
	    MEM_BOOTHEAP,
	    0,
	    bootinfo::ADR + bootinfo::MAX_BYTES,
	    false);
}

}  // namespace

cause::stype pre_load(u32 magic, const u32* tag)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		return cause::INVALID_PARAMS;

	// temporary
	vga_dev.init(80, 25, (void*)0xb8000);

	vga_log.attach(&vga_dev);
	log_set(0, &vga_log);

	log()("&tag : ")(&tag)(", tag : ")(tag);

	const u32* p = tag;
	const u32 tag_size = *p;
	p += 2;

	log()(", size : ").u(tag_size)();

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
			//log()("modules tag availavle.")();
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

	return cause::OK;
}

