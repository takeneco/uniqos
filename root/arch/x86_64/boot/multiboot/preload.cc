/// @file   preload.cc
/// @todo  自分自身を mem_alloc から除外していない。
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"
#include "misc.hh"
#include "multiboot2.h"
#include "vga.hh"


text_vga vga_dev;
log_file vga_log;

namespace {

void mem_setup(const multiboot_tag_mmap* mbt_mmap)
{
//	log()("memmap : esize=").u(u32(mbt_mmap->entry_size))
//	    (", eversion=").u(u32(mbt_mmap->entry_version))();

	const void* end = (const u8*)mbt_mmap + mbt_mmap->size;
	const multiboot_memory_map_t* mmap = mbt_mmap->entries;
	while (mmap < end) {
		/*
		log()(" ").u(u64(mmap->addr), 16)
		    (" len=").u(u64(mmap->len), 16)
		    (" type=").u(u32(mmap->type))();
		*/

		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
			const u64 TH = 0x1000000;
			u64 len = mmap->len;
			if (mmap->addr < TH && mmap->addr + mmap->len >= TH) {
				mem_add(
				    TH, mmap->addr + mmap->len - TH, false);
				len = TH - mmap->addr;
			}
			mem_add(mmap->addr, len, mmap->addr < 0x1000000);
		}
		mmap = (const multiboot_memory_map_t*)
		    ((const u8*)mmap + mbt_mmap->entry_size);
	}
}

}  // namespace

extern "C" cause::type pre_load(u32 magic, u32* tag)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		return cause::INVALID_PARAMS;

	vga_dev.init(80, 25, (void*)0xb8000);

	vga_log.attach(&vga_dev);
	log_set(0, &vga_log);
	log_set(1, &vga_log);

	mem_init();

	log()("&tag : ")(&tag);
	log()(", tag : ")(tag);

	u32 size = *tag;
	tag += 2;

	log()(", size : ").u(size)();

	u32 read = 8;
	while (read < size) {
		const multiboot_tag* mbt =
		    reinterpret_cast<const multiboot_tag*>(tag);
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
			mem_setup(mbt_mmap);
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
			read = size; // force loop break
			break;

		default:
			//log()("unknown type info : ").u(u32(mbt->type))();
			break;
		}

		const u32 dec = (mbt->size + 7) & ~7;
		tag += dec / sizeof *tag;
		read += dec;
	}

	return cause::OK;
}



