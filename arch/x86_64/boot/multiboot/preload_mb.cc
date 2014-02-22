/// @file   preload_mb.cc
/// @brief  Prepare to load kernel for multiboot1.

//  UNIQOS  --  Unique Operating System
//  (C) 2013-2014 KATO Takeshi
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

// multiboot と multiboot2 はインクルードガードがかぶっている
#include <multiboot.h>
#include <bootinfo.hh>
#include <config.h>
#include <multiboot.h>
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

void mem_setup(const multiboot_info* mbi)
{
	init_alloc();

	allocator* alloc = get_alloc();
	separator sep(alloc);
	init_sep(&sep);

	const u8* end = (const u8*)mbi->mmap_addr + mbi->mmap_length;
	const u8* cur = (const u8*)mbi->mmap_addr;
	while (cur < end) {
		const multiboot_memory_map_t* const mmap =
		    (const multiboot_memory_map_t*)cur;
		cur += sizeof mmap->size + mmap->size;

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
	}

	// self memory
	alloc->reserve(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP,
	    reinterpret_cast<uptr>(self_baseadr),
	    reinterpret_cast<uptr>(self_size),
	    true);

	// multiboot_info
	alloc->reserve(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL,
	    reinterpret_cast<uptr>(mbi),
	    sizeof *mbi,
	    true);

	// コンベンショナルメモリは multiboot のパラメータの格納に
	// 使われてしまう。
	alloc->reserve(SLOTM_CONVENTIONAL, 0, 0xfffff, false);
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

/// Convert multiboot_memory_map to bootinfo::adr_map
cause::t store_adrmap(const multiboot_info* mbi)
{
	allocator* alloc = get_alloc();

	// countup

	const u8* const end = (const u8*)mbi->mmap_addr + mbi->mmap_length;

	const u8* cur = (const u8*)mbi->mmap_addr;
	int ents = 0;
	while (cur < end) {
		const multiboot_memory_map_t* const mmap =
		    (const multiboot_memory_map_t*)cur;

		++ents;

		cur += sizeof mmap->size + mmap->size;
	}

	uptr adr_map_bytes = sizeof (bootinfo::adr_map) +
		             sizeof (bootinfo::adr_map::entry) * ents;

	bootinfo::adr_map* adrmap = static_cast<bootinfo::adr_map*>(
	    alloc->alloc(
	        SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL,
	        adr_map_bytes,
	        4096,
	        true));

	// store

	cur = (const u8*)mbi->mmap_addr;
	int adrmap_i = 0;
	while (cur < end) {
		const multiboot_memory_map_t* const mmap =
		    (const multiboot_memory_map_t*)cur;

		auto r = mbmmap_to_adrmap(mmap, &adrmap->entries[adrmap_i]);
		if (is_fail(r))
			return r;

		++adrmap_i;

		cur += sizeof mmap->size + mmap->size;
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
cause::t pre_load_mb(u32 magic, const void* tag)
{
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
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

	const multiboot_info* mbh = static_cast<const multiboot_info*>(tag);
	/*
	if(CONFIG_DEBUG_BOOT > 1) {
		log(1)("mbh          : ")(&mbh)
		    ("  flags        : 0x").x(mbh->flags)();
	}
	if (mbh->flags & MULTIBOOT_INFO_MEMORY) {
		// mem_lower, mem_upper are available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("mem_lower    : 0x").x(mbh->mem_lower)();
			log(1)("mem_upper    : 0x").x(mbh->mem_upper)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_BOOTDEV) {
		// boot_device is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("boot_device  : ").x(mbh->boot_device, 8)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_CMDLINE) {
		// cmdline is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("cmdline      : ").x(mbh->cmdline).c(' ').str(
			    reinterpret_cast<const char*>(mbh->cmdline))();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_MODS) {
		// mods_count, mods_addr are available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("mods_count   : ").u(mbh->mods_count)();
			log(1)("mods_addr    : 0x").x(mbh->mods_addr)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_AOUT_SYMS) {
		// aout_sym is available.
	}
	if (mbh->flags & MULTIBOOT_INFO_ELF_SHDR) {
		// elf_sec is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("ELF num      : ").u(mbh->u.elf_sec.num)();
			log(1)("ELF size     : 0x").x(mbh->u.elf_sec.size)();
			log(1)("ELF addr     : 0x").x(mbh->u.elf_sec.addr)();
			log(1)("ELF shndx    : ").x(mbh->u.elf_sec.shndx)();
		}
	}
	*/
	if (mbh->flags & MULTIBOOT_INFO_MEM_MAP) {
		// mmap_length, mmap_addr are available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("mmap_length  : 0x").x(mbh->mmap_length)();
			log(1)("mmap_addr    : 0x").x(mbh->mmap_addr)();
		}
		mem_setup(mbh);

		auto r = store_adrmap(mbh);
		if (is_fail(r))
			return r;
	}
	/*
	if (mbh->flags & MULTIBOOT_INFO_DRIVE_INFO) {
		// drives_length, drives_addr are available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("drives_length: 0x").x(mbh->mmap_length)();
			log(1)("drives_addr  : 0x").x(mbh->mmap_addr)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_CONFIG_TABLE) {
		// config_table is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("config_table : 0x").x(mbh->config_table)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
		// boot_loader_name is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("boot_loader_n: ").str(
			    reinterpret_cast<const char*>(mbh->boot_loader_name)
			)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_APM_TABLE) {
		// apm_table is available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("apm_table    : 0x").x(mbh->apm_table)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_VBE_INFO) {
		// vbe_* are available.
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("vbe_control_i: 0x").x(mbh->vbe_control_info)();
			log(1)("vbe_mode_info: 0x").x(mbh->vbe_mode_info)();
			log(1)("vbe_mode     : 0x").x(mbh->vbe_mode)();
			log(1)("vbe_inter_seg: 0x").x(mbh->vbe_interface_seg)();
			log(1)("vbe_inter_off: 0x").x(mbh->vbe_interface_off)();
			log(1)("vbe_inter_len: 0x").x(mbh->vbe_interface_len)();
		}
	}
	if (mbh->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
		if (CONFIG_DEBUG_BOOT > 1) {
			log(1)("frameb_addr  : 0x").x(mbh->framebuffer_addr)();
			log(1)("frameb_pitch : ").u(mbh->framebuffer_pitch)();
			log(1)("frameb_width : ").u(mbh->framebuffer_width)();
			log(1)("frameb_height: ").u(mbh->framebuffer_height)();
			log(1)("frameb_bpp   : ").u(mbh->framebuffer_bpp)();
			log(1)("frameb_type  : ").u(mbh->framebuffer_type)();
		}
	}
	*/

	r = memlog.open();
	if (is_fail(r))
		return r;

	return cause::OK;
}

