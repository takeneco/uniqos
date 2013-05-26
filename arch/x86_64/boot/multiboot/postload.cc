/// @file   postload.cc
/// @brief  カーネルへ渡すパラメータを作成する。

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2013 KATO Takeshi
//
//  uniqos is free software: you can redistribute it and/or modify
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
#include <string.hh>


extern u32 stack_start[];

extern const u8 first_process[];
extern const u8 first_process_size[];

namespace {

/// @brief  multiboot information を bootinfo にコピーする。
/// @param[in]  mb_info  multiboot information
/// @param[in]  bootinfo_left  bootinfo の残バッファサイズ
/// @param[out] bootinfo  コピー先 bootinfo のバッファ
/// @return  コピーしたサイズを返す。
///          バッファが足りない場合は、bootinfo_left より大きい値を返す。
uptr store_multiboot(const u32* mb_info, uptr bootinfo_left, u8* bootinfo)
{
	u32 size = *mb_info;
	if (size > bootinfo_left)
		return bootinfo_left + 1;

	mem_move(size, mb_info, bootinfo);

	return size;
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

	tag_ma->type = bootinfo::TYPE_MEMALLOC;
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
	tag_log->size = size;

	return size;
}

uptr store_first_process(uptr bootinfo_left, u8* bootinfo)
{
	multiboot_tag_module* tag_mod =
	    reinterpret_cast<multiboot_tag_module*>(bootinfo);

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
	tag_mod->size = bundle_size;
	tag_mod->mod_start = reinterpret_cast<u32>(p);
	tag_mod->mod_end = tag_mod->mod_start + bundle_size;
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
	uptr wrote = 0;

	uptr size = store_multiboot(mb_info, bootinfo_left, &bootinfo[wrote]);
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


cause::type post_load(u32* mb_info)
{
	if (!stack_test())
		return cause::FAIL;

	if (!store_bootinfo(mb_info)) {
		log(1)("Could not pass bootinfo to kernel.")();
		return cause::FAIL;
	}

	return cause::OK;
}

