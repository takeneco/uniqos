/// @file   postload.cc
/// @brief  カーネルへ渡すパラメータを作成する。

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
#include <string.hh>


extern u32 stack_start[];

extern const u8 first_process[];
extern const u8 first_process_size[];

namespace {

/// @brief multiboot_mmap_entry を bootinfo::adrmap に変換する
uptr store_adr_map(const u32* mb_info, uptr store_bytes, u8* store)
{
	if (store_bytes < adr_map_store->info_bytes)
		return adr_map_store->info_bytes;

	mem_copy(adr_map_store->info_bytes, adr_map_store, store);

	return adr_map_store->info_bytes;
}

/// @brief  multiboot information を bootinfo にコピーする。
/// @param[in]  mb_info  multiboot information
/// @param[in]  store_bytes  store の残バッファサイズ
/// @param[out] store  コピー先 bootinfo のバッファ
/// @return  コピーしたサイズを返す。
///          バッファが足りない場合は、store_bytes より大きい値を返す。
uptr store_multiboot(const u32* mb_info, uptr store_bytes, u8* store)
{
	u32 mb_bytes = *mb_info;

	u32 info_bytes = mb_bytes + sizeof (bootinfo::multiboot);

	u32 total_bytes = up_align<u32>(info_bytes, bootinfo::ALIGN);

	if (total_bytes > store_bytes)
		return info_bytes;

	bootinfo::multiboot* mbtag = (bootinfo::multiboot*)store;
	mbtag->info_type = bootinfo::TYPE_MULTIBOOT;
	mbtag->info_flags = bootinfo::multiboot::FLAG_2;
	mbtag->info_bytes = total_bytes;

	mem_move(mb_bytes, mb_info, mbtag->info);

	mem_fill(total_bytes - info_bytes, 0, &store[info_bytes]);

	return total_bytes;
}

/// @brief  使用中のメモリ情報を bootinfo に格納する。
/// @param[in]  store_bytes  store の残バッファサイズ
/// @param[out] store  コピー先 bootinfo のバッファ
/// @return  コピーしたサイズを返す。
///          バッファが足りない場合は、store_bytes より大きい値を返す。
uptr store_mem_alloc(uptr store_bytes, u8* store)
{
	bootinfo::mem_alloc* tag_ma =
	    reinterpret_cast<bootinfo::mem_alloc*>(store);

	uptr info_bytes = sizeof *tag_ma;
	if (store_bytes < info_bytes)
		return info_bytes;

	const allocator* alloc = get_alloc();

	allocator::enum_desc ea_enum;
	alloc->enum_alloc(
	    SLOTM_NORMAL | SLOTM_BOOTHEAP | SLOTM_CONVENTIONAL, &ea_enum);

	bootinfo::mem_alloc::entry* ma_ent = tag_ma->entries;
	for (;;) {
		u32 adr, bytes;
		const bool r = alloc->enum_alloc_next(&ea_enum, &adr, &bytes);
		if (!r)
			break;

		info_bytes += sizeof *ma_ent;
		if(store_bytes < info_bytes)
			return info_bytes;

		ma_ent->adr = adr;
		ma_ent->bytes = bytes;
		++ma_ent;
	}

	tag_ma->info_type = bootinfo::TYPE_MEM_ALLOC;
	tag_ma->info_flags = 0;
	tag_ma->info_bytes = info_bytes;

	return info_bytes;
}

uptr store_log(uptr store_bytes, u8* store)
{
	bootinfo::log* tag_log = reinterpret_cast<bootinfo::log*>(store);

	uptr info_bytes = sizeof *tag_log;
	if (store_bytes < info_bytes)
		return info_bytes;

	iovec iov;
	iov.base = tag_log->log;
	iov.bytes = store_bytes - info_bytes;
	io_node::offset read_bytes = 0;
	cause::t r = memlog.read(&read_bytes, 1, &iov);
	if (is_fail(r))
		return 0;

	if (read_bytes & (bootinfo::ALIGN - 1)) {
		int pads =
		    bootinfo::ALIGN - (read_bytes & (bootinfo::ALIGN - 1));
		for (int i = 0; i < pads; ++i) {
			if (iov.bytes <= read_bytes)
				break;
			static_cast<char*>(iov.base)[read_bytes++] = '_';
		}
	}

	info_bytes += read_bytes;

	tag_log->info_type = bootinfo::TYPE_LOG;
	tag_log->info_flags = 0;
	tag_log->info_bytes = info_bytes;

	return info_bytes;
}

uptr store_first_process(uptr store_bytes, u8* store)
{
	bootinfo::module* tag_mod =
	    reinterpret_cast<bootinfo::module*>(store);

	const uptr info_bytes = sizeof *tag_mod + sizeof (uptr);
	if (store_bytes < info_bytes)
		return info_bytes;

	const uptr bundle_size = reinterpret_cast<uptr>(first_process_size);

	void* p = get_alloc()->alloc(
	    SLOTM_NORMAL | SLOTM_CONVENTIONAL,
	    bundle_size,
	    4096,
	    false);

	mem_copy(bundle_size, first_process, p);

	tag_mod->info_type = bootinfo::TYPE_BUNDLE;
	tag_mod->info_flags = 0;
	tag_mod->info_bytes = info_bytes;
	tag_mod->mod_start = reinterpret_cast<u32>(p);
	tag_mod->mod_bytes = bundle_size;
	tag_mod->cmdline[0] = '\0';

	return info_bytes;
}

u8* bootinfo_alloc()
{
	void* p = get_alloc()->alloc(
	    SLOTM_CONVENTIONAL,
	    bootinfo::MAX_BYTES,
	    bootinfo::ALIGN,
	    false);

	if (!p) {
		p = get_alloc()->alloc(
		    SLOTM_BOOTHEAP,
		    bootinfo::MAX_BYTES,
		    bootinfo::ALIGN,
		    false);
	}

	return reinterpret_cast<u8*>(p);
}

bool store_bootinfo(const u32* mb_info)
{
	u8* const store = bootinfo_alloc();
	if (store == 0)
		return false;

	uptr store_bytes = bootinfo::MAX_BYTES;
	uptr wrote = sizeof (bootinfo::header);

	uptr size = store_adr_map(mb_info, store_bytes, &store[wrote]);
	if (size > store_bytes)
		return false;
	wrote += size;
	store_bytes -= size;

	size = store_multiboot(mb_info, store_bytes, &store[wrote]);
	if (size > store_bytes)
		return false;
	wrote += size;
	store_bytes -= size;

	size = store_mem_alloc(store_bytes, &store[wrote]);
	if (size > store_bytes)
		return false;
	wrote += size;
	store_bytes -= size;

	size = store_log(store_bytes, &store[wrote]);
	if (size > store_bytes)
		return false;
	wrote += size;
	store_bytes -= size;

	size = store_first_process(store_bytes, &store[wrote]);
	if (size > store_bytes)
		return false;
	wrote += size;
	store_bytes -= size;

	bootinfo::header* bootinfo_header =
	    reinterpret_cast<bootinfo::header*>(store);
	bootinfo_header->length = wrote;
	bootinfo_header->zero = 0;

	load_info.bootinfo_adr = reinterpret_cast<u64>(store);

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

