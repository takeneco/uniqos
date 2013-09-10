/// @file   postload.cc
/// @brief  カーネルへ渡すパラメータを作成する。

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
#include <string.hh>


extern u32 stack_start[];

extern const u8 first_process[];
extern const u8 first_process_size[];

namespace {

/// @brief multiboot_mmap_entry を bootinfo::adrmap に変換する
uptr store_adr_map(const u32* mb_info, uptr bootinfo_left, u8* bootinfo)
{
	bootinfo::adr_map* adrmap = (bootinfo::adr_map*)bootinfo;
	bootinfo::adr_map::entry* adrmap_ent = adrmap->entries;
	uptr data_size = sizeof (bootinfo::adr_map);

	//const u32 mb_info_size = *mb_info;
	mb_info += 2;

	// search multiboot_tag_mmap
	const multiboot_tag* mbt =
	    reinterpret_cast<const multiboot_tag*>(mb_info);
	while (mbt->type != MULTIBOOT_TAG_TYPE_MMAP) {
		const u32 inc = up_align<u32>(mbt->size, MULTIBOOT_TAG_ALIGN);
		mbt = (const multiboot_tag*)((const u8*)mbt + inc);

		if (mbt->type == MULTIBOOT_TAG_TYPE_END)
			return 0;
	}

	// store bootinfo::adr_map
	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)mbt;
	const multiboot_memory_map_t* mmap_ent = mmap->entries;
	const void* end = (const u8*)mmap + mmap->size;
	while (mmap_ent < end) {
		if (bootinfo_left <
		    data_size + sizeof (bootinfo::adr_map::entry))
		{
			return bootinfo_left + 1;
		}

		adrmap_ent->adr = mmap_ent->addr;
		adrmap_ent->len = mmap_ent->len;
		if (mmap_ent->type == MULTIBOOT_MEMORY_AVAILABLE)
			adrmap_ent->type = bootinfo::adr_map::entry::AVAILABLE;
		else if (mmap_ent->type == MULTIBOOT_MEMORY_RESERVED)
			adrmap_ent->type = bootinfo::adr_map::entry::RESERVED;
		else if (mmap_ent->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
			adrmap_ent->type = bootinfo::adr_map::entry::ACPI;
		else if (mmap_ent->type == MULTIBOOT_MEMORY_NVS)
			adrmap_ent->type = bootinfo::adr_map::entry::NVS;
		else if (mmap_ent->type == MULTIBOOT_MEMORY_BADRAM)
			adrmap_ent->type = bootinfo::adr_map::entry::BADRAM;
		adrmap_ent->zero = 0;

		data_size += sizeof (bootinfo::adr_map::entry);
		++adrmap_ent;
		mmap_ent = (const multiboot_memory_map_t*)
		    ((const u8*)mmap_ent + mmap->entry_size);
	}

	adrmap->type = bootinfo::TYPE_ADR_MAP;
	adrmap->flags = 0;
	adrmap->size = data_size;

	return data_size;
}

/// @brief  multiboot information を bootinfo にコピーする。
/// @param[in]  mb_info  multiboot information
/// @param[in]  bootinfo_left  bootinfo の残バッファサイズ
/// @param[out] bootinfo  コピー先 bootinfo のバッファ
/// @return  コピーしたサイズを返す。
///          バッファが足りない場合は、bootinfo_left より大きい値を返す。
uptr store_multiboot(const u32* mb_info, uptr bootinfo_left, u8* bootinfo)
{
	u32 size = *mb_info;
	if (size + sizeof (bootinfo::multiboot) > bootinfo_left)
		return bootinfo_left + 1;

	bootinfo::multiboot* mbtag = (bootinfo::multiboot*)bootinfo;
	mbtag->type = bootinfo::TYPE_MULTIBOOT;
	mbtag->flags = bootinfo::multiboot::FLAG_2;
	mbtag->size = size + sizeof (bootinfo::multiboot);

	mem_move(size, mb_info, mbtag->info);

	return size + sizeof (bootinfo::multiboot);
}

/// @brief  使用中のメモリ情報を bootinfo に格納する。
/// @param[in]  bootinfo_left  bootinfo の残バッファサイズ
/// @param[out] bootinfo  コピー先 bootinfo のバッファ
/// @return  コピーしたサイズを返す。
///          バッファが足りない場合は、bootinfo_left より大きい値を返す。
uptr store_mem_alloc(uptr bootinfo_left, u8* bootinfo)
{
	bootinfo::mem_alloc* tag_ma =
	    reinterpret_cast<bootinfo::mem_alloc*>(bootinfo);

	uptr size = sizeof *tag_ma;
	if (bootinfo_left < size)
		return size;

	const allocator* alloc = get_alloc();

	allocator::enum_desc ea_enum;
	alloc->enum_alloc(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL, &ea_enum);

	bootinfo::mem_alloc_entry* ma_ent = tag_ma->entries;
	for (;;) {
		u32 adr, bytes;
		const bool r = alloc->enum_alloc_next(&ea_enum, &adr, &bytes);
		if (!r)
			break;

		size += sizeof *ma_ent;
		if(bootinfo_left < size)
			return size;

		ma_ent->adr = adr;
		ma_ent->bytes = bytes;
		++ma_ent;
	}

	tag_ma->type = bootinfo::TYPE_MEM_ALLOC;
	//tag_ma->flags = 0;
	tag_ma->size = size;

	return size;
}

uptr store_log(uptr bootinfo_left, u8* bootinfo)
{
	bootinfo::log* tag_log = reinterpret_cast<bootinfo::log*>(bootinfo);

	uptr size = sizeof *tag_log;
	if (bootinfo_left < size)
		return size;

	iovec iov;
	iov.base = tag_log->log;
	iov.bytes = bootinfo_left - size;
	io_node::offset read_bytes = 0;
	cause::type r = memlog.read(&read_bytes, 1, &iov);
	if (is_fail(r))
		return 0;

	if (read_bytes & (MULTIBOOT_TAG_ALIGN - 1)) {
		int pads = MULTIBOOT_TAG_ALIGN -
		           (read_bytes & (MULTIBOOT_TAG_ALIGN - 1));
		for (int i = 0; i < pads; ++i) {
			if (iov.bytes <= read_bytes)
				break;
			static_cast<char*>(iov.base)[read_bytes++] = '_';
		}
	}

	size += read_bytes;

	tag_log->type = bootinfo::TYPE_LOG;
	tag_log->flags = 0;
	tag_log->size = size;

	return size;
}

uptr store_first_process(uptr bootinfo_left, u8* bootinfo)
{
	bootinfo::module* tag_mod =
	    reinterpret_cast<bootinfo::module*>(bootinfo);

	const uptr tag_size = sizeof *tag_mod + sizeof (uptr);
	if (bootinfo_left < tag_size)
		return tag_size;

	const uptr bundle_size = reinterpret_cast<uptr>(first_process_size);

	void* p = get_alloc()->alloc(
	    SLOTM_NORMAL | SLOTM_CONVENTIONAL,
	    bundle_size,
	    4096,
	    false);

	mem_copy(bundle_size, first_process, p);

	tag_mod->type = bootinfo::TYPE_BUNDLE;
	tag_mod->flags = 0;
	tag_mod->size = tag_size;
	tag_mod->mod_start = reinterpret_cast<u32>(p);
	tag_mod->mod_size = bundle_size;
	tag_mod->cmdline[0] = '\0';

	return tag_size;
}

u8* bootinfo_alloc()
{
	void* p = get_alloc()->alloc(
	    SLOTM_CONVENTIONAL,
	    bootinfo::MAX_BYTES,
	    MULTIBOOT_TAG_ALIGN,
	    false);

	if (!p) {
		p = get_alloc()->alloc(
		    SLOTM_BOOTHEAP,
		    bootinfo::MAX_BYTES,
		    MULTIBOOT_TAG_ALIGN,
		    false);
	}

	return reinterpret_cast<u8*>(p);
}

bool store_bootinfo(const u32* mb_info)
{
	u8* const bootinfo = bootinfo_alloc();
	if (bootinfo == 0)
		return false;

	uptr bootinfo_left = bootinfo::MAX_BYTES;
	uptr wrote = sizeof (bootinfo::header);

	uptr size = store_adr_map(mb_info, bootinfo_left, &bootinfo[wrote]);
	if (size > bootinfo_left)
		return false;
	wrote += size;
	bootinfo_left -= size;

	size = store_multiboot(mb_info, bootinfo_left, &bootinfo[wrote]);
	if (size > bootinfo_left)
		return false;
	wrote += size;
	bootinfo_left -= size;

	size = store_mem_alloc(bootinfo_left, &bootinfo[wrote]);
	if (size > bootinfo_left)
		return false;
	wrote += size;
	bootinfo_left -= size;

	size = store_log(bootinfo_left, &bootinfo[wrote]);
	if (size > bootinfo_left)
		return false;
	wrote += size;
	bootinfo_left -= size;

	size = store_first_process(bootinfo_left, &bootinfo[wrote]);
	if (size > bootinfo_left)
		return false;
	wrote += size;
	bootinfo_left -= size;

	// total size
	*reinterpret_cast<u32*>(bootinfo) = wrote;

	load_info.bootinfo_adr = reinterpret_cast<u64>(bootinfo);

	return true;
}

bool stack_test()
{
	u32 i;
	for (i = 0; stack_start[i] == 0 && i < 0xffff; ++i);

	log()("boot left stack : ").u((i - 1) * 4)();

	return i > 0;
}

}  // namespace


cause::t post_load(u32* mb_info)
{
	if (!stack_test())
		return cause::FAIL;

	if (!store_bootinfo(mb_info)) {
		log(1)("Could not pass bootinfo to kernel.")();
		return cause::FAIL;
	}

	return cause::OK;
}

