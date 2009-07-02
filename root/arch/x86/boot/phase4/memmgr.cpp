/* FILE : arch/x86/boot/phase4/memmgr.cpp
 * VER  : 0.0.2
 * LAST : 2009-06-25
 * (C) Kato.T 2009
 *
 * 初期化処理で使用する簡単なメモリ管理の実装。
 * 4GB以下のメモリを管理する。
 */

#include <cstddef>
#include "btypes.hpp"
#include "boot.h"

struct memmap_entry {
	memmap_entry* prev;
	memmap_entry* next;
	_u32 head;
	_u32 bytes; // bytes == 0 ならば未使用のインスタンス。
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
// 作業エリアの開始アドレスは NULL 禁止。
memmap_entry* memmap_buf
	= reinterpret_cast<memmap_entry*>(0x50000);

// 作業エリアのサイズ = 0x10000 bytes
const int memmap_buf_count
	= 0x10000 / sizeof(memmap_entry);

// 空き領域リスト
memmap_entry* free_list;

// 割り当て済み領域リスト
memmap_entry* nofree_list;

/**
 * 作業エリアを初期化する。
 */
static void memmap_buf_init()
{
	free_list = nofree_list = NULL;

	for (int i = 0; i < memmap_buf_count; i++) {
		memmap_buf[i].bytes = 0;
	}
}

/**
 * memmap_buf 配列から未使用の memmap_entry を探す。
 * @return 未使用の memmap_entry へのポインタを返す。
 * @return 未使用エントリがない場合は NULL を返す。
 */
static memmap_entry* memmap_new_entry()
{
	memmap_entry* p = memmap_buf;

	for (int i = 0; i < memmap_buf_count; i++) {
		if (p[i].bytes == 0) {
			return &p[i];
		}
	}
	return NULL;
}

/**
 * リストに memmap_entry を追加する。
 * @param list memmap_entry のリストへのポインタ。
 * @param ent  追加するエントリへのポインタ。
 */
static void memmap_insert_to(memmap_entry** list, memmap_entry* ent)
{
	if (!list || !ent)
		return;

	ent->prev = NULL;
	ent->next = *list;
	(*list)->prev = ent;
	*list = ent;
}

/**
 * memap_entry のリストからエントリを取り除く
 * @param list memmap_entry のリストへのポインタ。
 * @param ent  取り除くエントリへのポインタ。
 */
static void memmap_remove_from(memmap_entry** list, memmap_entry* ent)
{
	if (!list || !ent)
		return;

	if (ent->prev == NULL) {
		*list = ent->next;
	} else {
		ent->prev->next = ent->next;
	}

	if (ent->next != NULL) {
		ent->next->prev = ent->prev;
	}
}

/**
 * ACPI のメモリマップを free_list へ追加する。
 * @param raw ACPI が出力したメモリマップのエントリ
 */
static void memmap_add_entry(const acpi_memmap* raw)
{
	// 32ビットで扱えないアドレスは無視する。
	if (raw->base >= 0x00000000ffffffffULL)
		return;
	if (raw->length == 0)
		return;

	memmap_entry* ent = memmap_new_entry();
	if (ent == NULL)
		return;

	ent->head = raw->base;
	if ((raw->base + raw->length) > 0x00000000ffffffffULL)
		ent->bytes = 0xffffffff;
	else
		ent->bytes = raw->length;

	memmap_insert_to(&free_list, ent);
}

/**
 * phase3 が ACPI で取得したメモリマップを取り込む。
 */
static void memmap_import()
{
	const _u32 memmap_count
		= *reinterpret_cast<_u32*>(PH3_4_MEMMAP_COUNT);
	const acpi_memmap* rawmap
		= reinterpret_cast<acpi_memmap*>(PH3_4_MEMMAP);

	for (int i = 0; i < memmap_count; i++) {
		if (rawmap[i].type == acpi_memmap::MEMORY) {
			memmap_add_entry(&rawmap[i]);
		}
	}
}

/**
 * メモリ空間を free_list から取り除く。
 * @param r_head 取り除くメモリの先頭アドレス。
 * @param r_tail 取り除くメモリの終端アドレス。
 */
static void memmap_reserve(_u64 r_head, _u64 r_tail)
{
	r_tail += 1;

	memmap_entry* ent;
	for (ent = free_list; ent; ent = ent->next) {
		_u32 e_head = ent->head;
		_u32 e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			memmap_entry* ent2 = memmap_new_entry();
			ent2->head = r_tail;
			ent2->bytes = e_tail - r_tail;
			memmap_insert_to(&free_list, ent2);

			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head <= r_tail) {
			e_head = r_tail;
		}
		if (r_head <= e_tail && e_tail <= r_tail) {
			e_tail = r_head;
		}
		if (e_head >= e_tail) {
			memmap_remove_from(&free_list, ent);
			ent->bytes = 0;
		} else {
			ent->head = e_head;
			ent->bytes = e_tail - e_head;
		}
	}
}

/**
 * memmgr を初期化する。
 */
void memmgr_init()
{
	memmap_buf_init();

	memmap_import();

	// 先頭の３ＭＢを予約領域とする。
	// memmgr が NULL アドレスのメモリを確保することを防ぐ意味もある。
	memmap_reserve(0x00000000, 0x002fffff);
}

/**
 * メモリを確保する。
 * @param size 必要なメモリのサイズ。
 * @return 確保したメモリの先頭アドレスを返す。
 */
void* memmgr_alloc(size_t size)
{
	memmap_entry* ent;
	for (ent = free_list; ent; ent = ent->next) {
		if (ent->bytes >= size)
			break;
	}

	if (!ent)
		return NULL;

	_u32 addr;
	if (ent->bytes == size) {
		addr = ent->head;
		memmap_remove_from(&free_list, ent);
		memmap_insert_to(&nofree_list, ent);
	} else {
		memmap_entry* newent = memmap_new_entry();
		if (newent == NULL)
			return NULL;
		newent->head = ent->head;
		newent->bytes = size;
		memmap_insert_to(&nofree_list, newent);
		addr = newent->head;

		ent->head += size;
		ent->bytes -= size;
	}

	return reinterpret_cast<void*>(addr);
}

/**
 * メモリを解放する。
 * @param p 解放するメモリの先頭アドレス。
 */
void memmgr_free(void* p)
{
	// 割り当て済みリストから p を探す。

	_u32 head = reinterpret_cast<_u32>(p);
	memmap_entry* ent;
	for (ent = nofree_list; ent; ent = ent->next) {
		if (ent->head == head) {
			memmap_remove_from(&nofree_list, ent);
			break;
		}
	}

	if (!ent)
		return;

	// 空きリストから p の次のアドレスを探す。
	// p と p の次のアドレスを結合して ent とする。

	_u32 tail = head + ent->bytes;
	memmap_entry* ent2;
	for (ent2 = free_list; ent2; ent2 = ent2->next) {
		if (ent2->head == tail) {
			ent2->head = head;
			ent2->bytes += ent->bytes;
			memmap_remove_from(&free_list, ent2);
			ent->bytes = 0;
			ent = ent2;
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	head = ent->head;
	for (ent2 = free_list; ent2; ent2 = ent2->next) {
		if ((ent2->head + ent2->bytes) == head) {
			ent2->bytes += ent->bytes;
			ent->bytes = 0;
			ent = NULL;
			break;
		}
	}

	if (ent) {
		memmap_insert_to(&free_list, ent);
	}
}

