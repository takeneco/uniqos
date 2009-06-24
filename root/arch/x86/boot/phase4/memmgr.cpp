/* FILE : arch/x86/boot/phase4/memmgr.cpp
 * VER  : 0.0.1
 * LAST : 2009-06-24
 * (C) Kato.T 2009
 *
 * 初期化処理で使用する簡単なメモリ管理の実装。
 */

#include "types.hpp"
#include "boot.h"

struct memmap_entry {
	memmap_entry* next;
	_u32 enable; // 0 or 1
	_u64 start;
	_u64 bytes;
};

struct acpi_memmap {
	enum type_value {
		MEMORY = 1,
		RESERVED = 2,
		ACPI = 3,
		NVS = 4,
		UNUSUABLE = 5
	};
	_u64 base;
	_u64 length;
	_u32 type;
	_u32 attr;
};


// 作業エリア
memmap_entry* memmap_buf
	= reinterpret_cast<memmap_entry*>(0x50000);

// 作業エリアのサイズ = 0x10000 bytes
const int memmap_buf_count
	= 0x10000 / sizeof(memmap_entry);

// 空き領域リスト
memmap_entry* free_list = 0;

// 割り当て済み領域リスト
memmap_entry* nofree_list = 0;

/**
 * 作業エリアを初期化する。
 */
static void memmap_buf_init()
{
	for (int i = 0; i < memmap_buf_count; i++) {
		memmap_buf[i].enable = 0;
	}
}

/**
 * memmap_buf 配列から開いているエントリを探す。
 */
static memmap_entry* memmap_new_entry()
{
	memmap_entry* p = memmap_buf;

	for (int i = 0; i < memmap_buf_count; i++) {
		if (p[i].enable == 0) {
			p[i].enable = 1;
			return &p[i];
		}
	}
	return 0;
}

/**
 * ACPI のメモリマップを free_list へ追加する。
 */
static void memmap_add_entry(const acpi_memmap* raw)
{
	memmap_entry* ent = memmap_new_entry();
	if (ent == 0)
		return;

	ent->start = raw->base;
	ent->bytes = raw->length;

	ent->next = free_list;
	free_list = ent;
}

/**
 * ACPI で取得したメモリマップを取り込む。
 */
static void memmap_import()
{
	const _u32 memmap_count
		= *reinterpret_cast<_u32*>(PARAM_MEMMAP_COUNT);
	const acpi_memmap* rawmap
		= reinterpret_cast<acpi_memmap*>(PARAM_MEMMAP);

	for (int i = 0; i < memmap_count; i++) {
		if (rawmap[i].type == acpi_memmap::MEMORY) {
			memmap_add_entry(&rawmap[i]);
		}
	}
}

/**
 * 初期化する。
 */
void memmgr_init()
{
	memmap_buf_init();

	memmap_import();
}

