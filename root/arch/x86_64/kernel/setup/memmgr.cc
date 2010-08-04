// @file    arch/x86_64/kernel/setup/memmgr.cpp
// @author  Kato Takeshi
// @brief   Easy memory management implement used by setup.
//
// (C) 2010 Kato Takeshi.

#include "mem.hh"
#include "access.hh"


namespace {

const std::size_t MEMORY_MAX = 0x3fffffff; /* 1GB */

// Work area.
memmap_entry* const memmap_buf
	= reinterpret_cast<memmap_entry*>(SETUP_MEMMGR_ADR);

// Size of work area.
const int memmap_buf_count
	= SETUP_MEMMGR_SIZE / sizeof (memmap_entry);


/**
 * @brief  Initialize work area.
 */
void memmap_buf_init()
{
	for (int i = 0; i < memmap_buf_count; i++) {
		memmap_buf[i].bytes = 0;
	}
}

/**
 * @brief  Search unused memmap_entry from memmap_buf.
 * @return  If unused memmap_entry found, this func return ptr
 * @return  to unused memmap_entry.
 * @return  If unused memmap_entry not found, this func return NULL.
 */
memmap_entry* memmap_new_entry()
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
 * @brief  Add memmap_entry to chain.
 * @param list Ptr to memmap_entry chain ptr.
 * @param ent  Ptr to memmap_entry adds.
 */
void memmap_insert_to(memmap_entry** list, memmap_entry* ent)
{
	if (list == NULL || ent == NULL) {
		return;
	}

	ent->prev = NULL;
	ent->next = *list;
	(*list)->prev = ent;
	*list = ent;
}

/**
 * @brief  Remove memmap_entry from chain.
 * @param list Ptr to memmap_entry chain ptr.
 * @param ent  Ptr to memmap_entry removes.
 */
void memmap_remove_from(memmap_entry** list, memmap_entry* ent)
{
	if (list == NULL || ent == NULL) {
		return;
	}

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
 * @brief  Add free memory info to free_list from acpi_memmap.
 *
 * Excludes addr bigger than MEMORY_MAX.
 */
void memmap_add_entry(memmgr* mm, const acpi_memmap* raw)
{
	if (raw->length == 0) {
		return;
	}

	if (raw->base > MEMORY_MAX) {
		return;
	}

	memmap_entry* ent = memmap_new_entry();
	if (ent == NULL) {
		return;
	}

	ent->head = raw->base;

	if ((raw->base + raw->length) > MEMORY_MAX)
		ent->bytes = MEMORY_MAX - ent->head;
	else
		ent->bytes = raw->base + raw->length - ent->head;

	memmap_insert_to(&mm->free_list, ent);
}

/**
 * @brief  Import memory map by ACPI in real mode.
 */
void memmap_import(memmgr* mm)
{
	const acpi_memmap* rawmap = setup_get_ptr<acpi_memmap>(SETUP_MEMMAP);
	const u32 memmap_count = setup_get_value<u32>(SETUP_MEMMAP_COUNT);

	for (u32 i = 0; i < memmap_count; i++) {
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
void memmap_reserve(memmgr* mm, u64 r_head, u64 r_tail)
{
	r_tail += 1;

	memmap_entry* ent;
	for (ent = mm->free_list; ent; ent = ent->next) {
		u32 e_head = ent->head;
		u32 e_tail = e_head + ent->bytes;

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

}  // End of anonymous namespace

/**
 * @brief  Initialize memmgr.
 */
void memmgr_init(memmgr* mm)
{
	mm->free_list = mm->nofree_list = NULL;

	memmap_buf_init();

	memmap_import(mm);

	// 先頭の固定利用アドレスを予約領域とする。
	// memmgr が NULL アドレスのメモリを確保することを防ぐ意味もある。
	memmap_reserve(mm, 0x00000000, SETUP_MEMMGR_RESERVED_PADR);
}

/**
 * @brief  Allocate memory.
 * @param size Memory bytes.
 * @param align Memory aliments. Must to be 2^n.
 *              If align == 256, then return address is "0x....00".
 * @return If succeeds, this func returns allocated memory address ptr.
 * @return If fails, this func returns NULL.
 */
void* memmgr_alloc(memmgr* mm, std::size_t size, std::size_t align)
{
	memmap_entry* ent;
	for (ent = mm->free_list; ent; ent = ent->next) {
		u64 align_gap = up_align(align, ent->head) - ent->head;
		if ((ent->bytes - align_gap) < size)
			continue;

		u64 addr;
		if (ent->bytes == size) {
			addr = ent->head;
			memmap_remove_from(&mm->free_list, ent);
			memmap_insert_to(&mm->nofree_list, ent);
		} else {
			memmap_entry* newent = memmap_new_entry();
			if (newent == NULL)
				continue;
			if (align_gap != 0) {
				newent->head = ent->head;
				newent->bytes = align_gap;
				memmap_entry* newent2 = memmap_new_entry();
				if (newent2 == NULL) {
					newent->bytes = 0;
					continue;
				}
				newent = newent2;
				ent->head += align_gap;
				ent->bytes -= align_gap;
			}
			addr = newent->head = ent->head;
			newent->bytes = size;
			memmap_insert_to(&mm->nofree_list, newent);

			ent->head += size;
			ent->bytes -= size;
		}

		return reinterpret_cast<void*>(addr);
	}

	return NULL;
}

/**
 * @brief  Free memory.
 * @param p Ptr to memory frees.
 */
void memmgr_free(memmgr* mm, void* p)
{
	// 割り当て済みリストから p を探す。

	u64 head = reinterpret_cast<u64>(p);
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

// @brief  空きメモリアドレスをカーネルに渡すためにメモリ上へ出力する。
// @param[out] dumpto  Ptr to destination.
// @param[in] n        dumpto entries.
// @return  Dumped number.
int memmgr_dump(const memmgr* mm, memmgr_dumpdata* dumpto, int n)
{
	const memmap_entry* ent = mm->free_list;
	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto->head = ent->head;
		dumpto->bytes = ent->bytes;
		ent = ent->next;
	}

	return i;
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
