/// @author  Kato Takeshi
/// @brief   Easy memory management implement used by setup.
///
/// (C) 2010 Kato Takeshi

#include "mem.hh"
#include "memdump.hh"
#include "access.hh"
#include "pnew.hh"


namespace {

// Work area.
memmap_entry* const _memmap_buf
	= reinterpret_cast<memmap_entry*>(SETUP_MEMMGR_ADR);
enum {
// Size of work area.
	MEMMAP_BUF_COUNT = SETUP_MEMMGR_SIZE / sizeof (memmap_entry),
};

enum {
	MEMMGR_ALLOC_MAX = 0x4000000, // 64MiB
};

char _memmgr_buf[sizeof (memmgr)];
inline memmgr* _get_memmgr() { return reinterpret_cast<memmgr*>(_memmgr_buf); }

/// @brief  Initialize work area.
void _memmap_buf_init()
{
	for (int i = 0; i < MEMMAP_BUF_COUNT; i++)
		_memmap_buf[i].unset();
}

/// @brief  Search unused memmap_entry from memmap_buf.
/// @return  If unused memmap_entry found, this func return ptr
/// @return  to unused memmap_entry.
/// @return  If unused memmap_entry not found, this func return NULL.
memmap_entry* _memmap_new_entry()
{
	memmap_entry* p = _memmap_buf;

	for (int i = 0; i < MEMMAP_BUF_COUNT; i++) {
		if (p[i].bytes == 0) {
			p[i].bytes = 1;  // means using.
			return &p[i];
		}
	}
	return NULL;
}

/// @brief  Add free memory info to free_chain from acpi_memmap.
void _memmap_add_entry(const acpi_memmap* raw)
{
	if (raw->length == 0) {
		return;
	}

	memmap_entry* ent = _memmap_new_entry();
	if (ent == NULL) {
		return;
	}

	ent->set(raw->base, raw->length);

	_get_memmgr()->free_chain.insert_head(ent);
}

/**
 * @brief  Import memory map by ACPI in real mode.
 */
void _memmap_import()
{
	const acpi_memmap* rawmap =
	    setup_get_ptr<acpi_memmap>(SETUP_ACPI_MEMMAP);
	const int memmap_count =
	    setup_get_value<u32>(SETUP_ACPI_MEMMAP_COUNT);

	for (int i = 0; i < memmap_count; i++) {
		if (rawmap[i].type == acpi_memmap::MEMORY) {
			_memmap_add_entry(&rawmap[i]);
		}
	}
}

/**
 * メモリ空間を memmgr->free_chain から取り除く。
 * @param r_head 取り除くメモリの先頭アドレス。
 * @param r_tail 取り除くメモリの終端アドレス。
 */
void _memmap_reserve(u64 r_head, u64 r_tail)
{
	memmgr* const mm = _get_memmgr();

	r_tail += 1;

	for (memmap_entry* ent = mm->free_chain.get_head();
	     ent;
	     ent = mm->free_chain.get_next(ent))
	{
		u32 e_head = ent->head;
		u32 e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			memmap_entry* ent2 = _memmap_new_entry();
			ent2->set(r_tail, e_tail - r_tail);
			mm->free_chain.insert_head(ent2);

			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head <= r_tail)
			e_head = r_tail;
		if (r_head <= e_tail && e_tail <= r_tail)
			e_tail = r_head;

		if (e_head >= e_tail) {
			mm->free_chain.remove(ent);
			ent->unset();
		} else {
			ent->set(e_head, e_tail - e_head);
		}
	}
}

}  // End of anonymous namespace

/**
 * @brief  Initialize memmgr.
 */
void memmgr_init()
{
	new (_get_memmgr()) memmgr;

	_memmap_buf_init();

	_memmap_import();

	// 先頭の固定利用アドレスを予約領域とする。
	// memmgr が NULL アドレスのメモリを確保することを防ぐ意味もある。
	_memmap_reserve(0x00000000, SETUP_MEMMGR_RESERVED_PADR);
}

/// @brief  Allocate memory.
/// @param[in] size Memory bytes.
/// @param[in] align Memory aliments. Must to be 2^n.
///     If align == 256, then return address is "0x....00".
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns NULL.
void* memmgr_alloc(std::size_t size, std::size_t align)
{
	memmgr* const mm = _get_memmgr();

	for (memmap_entry* ent = mm->free_chain.get_head();
	     ent;
	     ent = mm->free_chain.get_next(ent)) {
		const u64 align_gap = up_align(align, ent->head) - ent->head;
		if ((ent->bytes - align_gap) < size)
			continue;

		u64 r;
		if (ent->bytes == size) {
			r = ent->head;
			mm->free_chain.remove(ent);
			mm->nofree_chain.insert_head(ent);
		} else {
			memmap_entry* newent = _memmap_new_entry();
			if (newent == NULL)
				continue;

			if (align_gap != 0) {
				memmap_entry* newent2 = _memmap_new_entry();
				if (newent2 == NULL) {
					newent->unset();
					continue;
				}
				newent2->set(ent->head, align_gap);
				mm->free_chain.insert_head(newent2);

				ent->head += align_gap;
				ent->bytes -= align_gap;
			}

			r = ent->head;
			newent->set(ent->head, size);
			mm->nofree_chain.insert_head(newent);

			ent->head += size;
			ent->bytes -= size;
		}

		return reinterpret_cast<void*>(r);
	}

	return NULL;
}

/// @brief  Free memory.
/// @param p Ptr to memory frees.
void memmgr_free(void* p)
{
	memmgr* const mm = _get_memmgr();

	// 割り当て済みリストから p を探す。

	u64 head = reinterpret_cast<u64>(p);
	memmap_entry* ent;
	for (ent = mm->nofree_chain.get_head();
	     ent;
	     ent = mm->nofree_chain.get_next(ent))
	{
		if (ent->head == head) {
			mm->nofree_chain.remove(ent);
			break;
		}
	}

	if (!ent)
		return;

	// 空きリストから p の次のアドレスを探す。
	// p と p の次のアドレスを結合して ent とする。

	u64 tail = head + ent->bytes;
	for (memmap_entry* ent2 = mm->free_chain.get_head();
	     ent2;
	     ent2 = mm->free_chain.get_next(ent2))
	{
		if (ent2->head == tail) {
			ent->bytes += ent2->bytes;
			mm->free_chain.remove(ent2);
			ent2->unset();
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	head = ent->head;
	for (memmap_entry* ent2 = mm->free_chain.get_head();
	     ent2;
	     ent2 = mm->free_chain.get_next(ent2))
	{
		if ((ent2->head + ent2->bytes) == head) {
			ent2->bytes += ent->bytes;
			ent->unset();
			ent = NULL;
			break;
		}
	}

	if (ent)
		mm->free_chain.insert_head(ent);
}

// @brief  空きメモリアドレスをカーネルに渡すためにメモリ上へ出力する。
// @param[out] dumpto  Ptr to destination.
// @param[in] n        dumpto entries.
// @return  Dumped number.
int memmgr_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	const memmgr* const mm = _get_memmgr();

	const memmap_entry* ent = mm->free_chain.get_head();
	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = mm->free_chain.get_next(ent);
	}

	return i;
}

