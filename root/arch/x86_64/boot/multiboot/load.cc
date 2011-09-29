

#include "log.hh"
#include "multiboot2.h"
#include "vga.hh"

extern u8 core[];
extern u8 core_size[];

extern "C" void pre(u32* tag)
{
	text_vga tv;
	tv.init(80, 25, (void*)0xb8000);
	log_file lf(&tv);
	log_set(0, &lf);
	log_set(1, &lf);

	u32 size = *tag;
	tag += 2;

	log()("size : ").u(size)();

	u32 read = 8;
	while (read < size) {
		const multiboot_tag* mbt =
		    reinterpret_cast<const multiboot_tag*>(tag);
		switch (mbt->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE: {
			const multiboot_tag_string* mbt_cmdline =
			    reinterpret_cast<const multiboot_tag_string*>(mbt);
			log()("cmdline : [")(mbt_cmdline->string)("]")();
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
			const multiboot_tag_string* mbt_bootldr =
			    reinterpret_cast<const multiboot_tag_string*>(mbt);
			log()("bootldr : [")(mbt_bootldr->string)("]")();
			break;
		}
		case MULTIBOOT_TAG_TYPE_MODULE:
			log()("modules tag availavle.")();
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
			const multiboot_tag_basic_meminfo* mbt_bmem =
			    reinterpret_cast<const multiboot_tag_basic_meminfo*>
			    (mbt);
			log()("basic memory : lower=").
			    u(u32(mbt_bmem->mem_lower))("KB, upper=").
			    u(u32(mbt_bmem->mem_upper))("KB")();
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOTDEV: {
			const multiboot_tag_bootdev* mbt_bootdev =
			    reinterpret_cast<const multiboot_tag_bootdev*>(mbt);
			log()("bios boot device : ").
			    u(u32(mbt_bootdev->biosdev), 16)(", ").
			    u(u32(mbt_bootdev->slice), 16)(", ").
			    u(u32(mbt_bootdev->part), 16)();
			break;
		}
		case MULTIBOOT_TAG_TYPE_MMAP: {
			const multiboot_tag_mmap* mbt_mmap =
			    reinterpret_cast<const multiboot_tag_mmap*>(mbt);
			log()("memmap : entry size=").
			    u(u32(mbt_mmap->entry_size))
			    (" entry version=").
			    u(u32(mbt_mmap->entry_version))();
			const void* end = (const u8*)mbt + mbt->size;
			const multiboot_memory_map_t* mmap = mbt_mmap->entries;
			while (mmap < end) {
				log()(" ").u(u64(mmap->addr), 16)
				    (" len=").u(u64(mmap->len), 16)
				    (" type=").u(u32(mmap->type))();
				mmap = (const multiboot_memory_map_t*)
				    ((const u8*)mmap + mbt_mmap->entry_size);
			}
			break;
		}
		case MULTIBOOT_TAG_TYPE_END:
			read = size;
			break;
		default:
			log()("unknown type info : ").u(u32(mbt->type))();
			break;
		}

		const u32 dec = (mbt->size + 7) & ~7;
		tag += dec/4;
		read += dec;
	}

	log()("core : ")(core)();
	log()("core_size : ")(core_size)();

	for (int i = 0; i < 20; ++i) {
		log().u(core[i], 16)(" ");
	}
}
