/// @file   arch/x86_64/boot/multiboot/misc.hh

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

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include <bootinfo.hh>
#include <cheap_alloc.hh>
#include <output_buffer.hh>


// 作業用バッファのエントリ数を 256 とする。大きすぎるとスタックに置けない。
typedef cheap_alloc<256> allocator;
typedef cheap_alloc_separator<allocator> separator;

enum MEM_SLOT {
	SLOT_INDEX_NORMAL = 0,

	// カーネルへ jmp した直後からカーネルがメモリをセットアップするまで
	// の間に RAM として使用可能なヒープを BOOTHEAP と呼ぶことにする。
	// メモリ管理上は BOOTHEAP を区別し、可能な限り空いたままにしておく。
	SLOT_INDEX_BOOTHEAP = 1,

	SLOT_INDEX_CONVENTIONAL = 2,
};
enum MEM_SLOT_MASK {
	SLOTM_NORMAL       = 1 << SLOT_INDEX_NORMAL,
	SLOTM_BOOTHEAP     = 1 << SLOT_INDEX_BOOTHEAP,
	SLOTM_CONVENTIONAL = 1 << SLOT_INDEX_CONVENTIONAL,
};

void  init_alloc();
allocator* get_alloc();

cause::t pre_load_mb2(u32 magic, const u32* tag);
cause::t pre_load_mb(u32 magic, const void* tag);
cause::t post_load(u32* tag);

extern struct load_info_
{
	u64 entry_adr;       // 0x00
	u64 stack_adr;       // 0x08
	u64 page_table_adr;  // 0x10

	u64 bootinfo_adr;    // 0x18
} load_info;


// log

class log : public output_buffer
{
	DISALLOW_COPY_AND_ASSIGN(log);

public:
	log(int i = 0);
	~log();
};

void log_set(uint i, io_node* target);


// memlog_file

class memlog_file : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(memlog_file);

	friend class io_node;

public:
	static cause::type setup();

	memlog_file() {}

	cause::t open();
	cause::t close();

	cause::t on_io_node_seek(
	    seek_whence whence, offset rel_off, offset* abs_off);
	cause::t on_io_node_read(
	    offset* off, int iov_cnt, iovec* iov);
	cause::t on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	u8* buf;
	offset size;
};

extern memlog_file memlog;
extern bootinfo::adr_map* adr_map_store;
extern bootinfo::mem_work* mem_work_store;


#endif  // include guard

