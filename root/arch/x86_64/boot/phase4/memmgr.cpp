/**
 * @file    arch/x86/boot/phase4/memmgr.cpp
 * @version 0.0.4
 * @date    2009-07-26
 * @author  Kato.T
 *
 * 初期化処理で使用する簡単なメモリ管理の実装。
 * 4GB以下のメモリを管理する。
 */
// (C) Kato.T 2009

#include <cstddef>
#include "btypes.hpp"
#include "boot.h"
#include "phase4.hpp"


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
static memmap_entry* const memmap_buf
	= reinterpret_cast<memmap_entry*>(PH4_MEMMAP_BUF);

// 作業エリアのサイズ = 0x10000 bytes
static const int memmap_buf_count
	= 0x10000 / sizeof(memmap_entry);

/**
 * プラス方向 align
 */
static inline _u32 up_align(_u32 x)
{
	return (x + 15) & 0xfffffff0;
}

/**
 * 作業エリアを初期化する。
 */
static void memmap_buf_init()
{
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
static void memmap_add_entry(memmgr* mm, const acpi_memmap* raw)
{
	// 32ビットで扱えないアドレスは無視する。
	if (raw->base >= 0x00000000ffffffffULL)
		return;
	if (raw->length == 0)
		return;

	memmap_entry* ent = memmap_new_entry();
	if (ent == NULL)
		return;

	ent->head = up_align(raw->base);
	if ((raw->base + raw->length) > 0x00000000ffffffffULL)
		ent->bytes = 0xffffffff - ent->head;
	else
		ent->bytes = raw->base + raw->length - ent->head;

	memmap_insert_to(&mm->free_list, ent);
}

/**
 * phase3 が ACPI で取得したメモリマップを取り込む。
 */
static void memmap_import(memmgr* mm)
{
	const _u32 memmap_count = *reinterpret_cast<_u32*>
		((PH3_4_PARAM_SEG << 4) + PH3_4_MEMMAP_COUNT);
	const acpi_memmap* rawmap = reinterpret_cast<acpi_memmap*>
		((PH3_4_PARAM_SEG << 4) + PH3_4_MEMMAP);

	for (int i = 0; i < memmap_count; i++) {
		if (rawmap[i].type == acpi_memmap::MEMORY) {
			memmap_add_entry(mm, &rawmap[i]);
		}
	}
}

/**
 * メモリ空間を memmgr->free_list から取り除く。
 * @param r_head 取り除くメモリの先頭アドレス。
 * @param r_tail 取り除くメモリの終端アドレス。
 */
static void memmap_reserve(memmgr* mm, _u64 r_head, _u64 r_tail)
{
	r_tail += 1;

	memmap_entry* ent;
	for (ent = mm->free_list; ent; ent = ent->next) {
		_u32 e_head = ent->head;
		_u32 e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			memmap_entry* ent2 = memmap_new_entry();
			ent2->head = r_tail;
			ent2->bytes = e_tail - r_tail;
			memmap_insert_to(&mm->free_list, ent2);

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
			memmap_remove_from(&mm->free_list, ent);
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
void memmgr_init(memmgr* mm)
{
	mm->free_list = mm->nofree_list = NULL;

	memmap_buf_init();

	memmap_import(mm);

	// 先頭の３ＭＢを予約領域とする。
	// memmgr が NULL アドレスのメモリを確保することを防ぐ意味もある。
	memmap_reserve(mm, 0x00000000, 0x002fffff);
}

/**
 * メモリを割り当てる。
 * @param size 必要なメモリのサイズ。
 * @return 確保したメモリの先頭アドレスを返す。
 */
void* memmgr_alloc(memmgr* mm, std::size_t size)
{
	size = up_align(size);

	memmap_entry* ent;
	for (ent = mm->free_list; ent; ent = ent->next) {
		if (ent->bytes >= size)
			break;
	}

	if (!ent)
		return NULL;

	_u32 addr;
	if (ent->bytes == size) {
		addr = ent->head;
		memmap_remove_from(&mm->free_list, ent);
		memmap_insert_to(&mm->nofree_list, ent);
	} else {
		memmap_entry* newent = memmap_new_entry();
		if (newent == NULL)
			return NULL;
		newent->head = ent->head;
		newent->bytes = size;
		memmap_insert_to(&mm->nofree_list, newent);
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
void memmgr_free(memmgr* mm, void* p)
{
	// 割り当て済みリストから p を探す。

	_ucpu head = reinterpret_cast<_ucpu>(p);
	memmap_entry* ent;
	for (ent = mm->nofree_list; ent; ent = ent->next) {
		if (ent->head == head) {
			memmap_remove_from(&mm->nofree_list, ent);
			break;
		}
	}

	if (!ent)
		return;

	// 空きリストから p の次のアドレスを探す。
	// p と p の次のアドレスを結合して ent とする。

	_ucpu tail = head + ent->bytes;
	memmap_entry* ent2;
	for (ent2 = mm->free_list; ent2; ent2 = ent2->next) {
		if (ent2->head == tail) {
			ent2->head = head;
			ent2->bytes += ent->bytes;
			memmap_remove_from(&mm->free_list, ent2);
			ent->bytes = 0;
			ent = ent2;
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	head = ent->head;
	for (ent2 = mm->free_list; ent2; ent2 = ent2->next) {
		if ((ent2->head + ent2->bytes) == head) {
			ent2->bytes += ent->bytes;
			ent->bytes = 0;
			ent = NULL;
			break;
		}
	}

	if (ent) {
		memmap_insert_to(&mm->free_list, ent);
	}
}

void* operator new(std::size_t s, memmgr* mm)
{
	void* p1 = memmgr_alloc(mm, s + sizeof (memmgr*));
	memmgr** p2 = reinterpret_cast<memmgr**>(p1);
	*p2 = mm;
	return p2 + 1;
}

void* operator new[](std::size_t s, memmgr* mm)
{
	void* p1 = memmgr_alloc(mm, s + sizeof (memmgr*));
	memmgr** p2 = reinterpret_cast<memmgr**>(p1);
	*p2 = mm;
	return p2 + 1;
}

void operator delete(void* p)
{
	memmgr** p1 = reinterpret_cast<memmgr**>(p);
	memmgr_free(p1[-1], p);
}

void operator delete[](void* p)
{
	memmgr** p1 = reinterpret_cast<memmgr**>(p);
	memmgr_free(p1[-1], p);
}
