/// @file  memmgr.cc
/// @brief Easy memory management implement used by setup.
//
// (C) 2010 KATO Takeshi
//

#include "chain.hh"
#include "memdump.hh"
#include "misc.hh"
#include "access.hh"
#include "placement_new.hh"

#include "native_ops.hh"

#include "term.hh"
extern term_chain* debug_tc;

namespace {

struct alloc_entry
{
	u64 head;
	u64 bytes;  ///< unused if bytes == 0.

	bichain_link<alloc_entry> chain_link_;

	bichain_link<alloc_entry>& chain_hook() {
		return chain_link_;
	}

	void set(u64 head_, u64 bytes_) {
		head = head_;
		bytes = bytes_;
	}
	void unset() {
		bytes = 0;
	}
};

class memmgr
{
	typedef bichain<alloc_entry, &alloc_entry::chain_hook>
	    alloc_entry_chain;

	/// 空き領域リスト
	alloc_entry_chain free_chain;

	/// 割り当て済み領域リスト
	alloc_entry_chain nofree_chain;

	static alloc_entry* const memmap_buf;

public:
	void init();
	alloc_entry* new_entry();
	void add_free_entry(const acpi_memmap* raw);
	void import();
	void reserve_range(u64 r_head, u64 r_tail);
	int freemem_dump(setup_memmgr_dumpdata* dumpto, int n);
	int nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n);
	void* memmgr_alloc(uptr size, uptr align);
	void memmgr_free(void* p);
};

// Work area.
alloc_entry* const memmgr::memmap_buf
	= reinterpret_cast<alloc_entry*>(SETUP_MEMMGR_ADR);
enum {
	// Size of work area.
	MEMMAP_BUF_COUNT = SETUP_MEMMGR_SIZE / sizeof (alloc_entry),
};

uptr memmgr_buf_[sizeof (memmgr) / sizeof (uptr)];
memmgr* memmgr_ptr;
//inline memmgr* get_memmgr() { return reinterpret_cast<memmgr*>(memmgr_buf_); }
inline memmgr* get_memmgr() { return memmgr_ptr; }

/// @brief  Initialize work area.
void memmgr::init()
{
	for (int i = 0; i < MEMMAP_BUF_COUNT; i++)
		memmap_buf[i].unset();
}

/// @brief  Search unused alloc_entry from memmap_buf.
/// @return  If unused alloc_entry found, this func returns ptr
/// @return  to unused alloc_entry.
/// @return  If unused alloc_entry not found, this func returns 0.
alloc_entry* memmgr::new_entry()
{
	alloc_entry* p = memmap_buf;

	for (int i = 0; i < MEMMAP_BUF_COUNT; ++i) {
		if (p[i].bytes == 0) {
			p[i].bytes = 1;  // means using.
			return &p[i];
		}
	}

	return 0;
}

/// @brief  Add free memory info to free_chain from acpi_memmap.
void memmgr::add_free_entry(const acpi_memmap* raw)
{
	if (raw->length == 0)
		return;

	alloc_entry* ent = new_entry();
	if (ent == 0)
		return;

	ent->set(raw->base, raw->length);

	free_chain.insert_head(ent);
}

/// @brief  Import memory map by ACPI in real mode.
void memmgr::import()
{
	const acpi_memmap* rawmap =
	    setup_get_ptr<acpi_memmap>(SETUP_ACPI_MEMMAP);
	const u32 memmap_count =
	    setup_get_value<u32>(SETUP_ACPI_MEMMAP_COUNT);

	for (u32 i = 0; i < memmap_count; i++) {
		if (rawmap[i].type == acpi_memmap::MEMORY)
			add_free_entry(&rawmap[i]);
	}
}

/// @brief メモリ空間を memmgr->free_chain から取り除く。
/// @param[in] r_head 取り除くメモリの先頭アドレス。
/// @param[in] r_tail 取り除くメモリの終端アドレス。
void memmgr::reserve_range(u64 r_head, u64 r_tail)
{
	r_tail += 1;

	for (alloc_entry* ent = free_chain.get_head();
	     ent;
	     ent = free_chain.get_next(ent))
	{
		uptr e_head = ent->head;
		uptr e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			alloc_entry* ent2 = new_entry();
			ent2->set(r_tail, e_tail - r_tail);
			free_chain.insert_head(ent2);

			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head <= r_tail)
			e_head = r_tail;
		if (r_head <= e_tail && e_tail <= r_tail)
			e_tail = r_head;

		if (e_head >= e_tail) {
			free_chain.remove(ent);
			ent->unset();
		} else {
			ent->set(e_head, e_tail - e_head);
		}
	}
}

/// @brief  空きメモリ情報をカーネルに渡すためにメモリ上へ出力する。
/// @param[out] dumpto  Ptr to dump destination.
/// @param[in] n        dumpto entries.
/// @return  Dumped count.
int memmgr::freemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	const memmgr* const mm = get_memmgr();

	const alloc_entry* ent = mm->free_chain.get_head();
	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = mm->free_chain.get_next(ent);
	}

	return i;
}

/// @brief  使用メモリ情報をカーネルに渡すためにメモリ上へ出力する。
/// @param[out] dumptp  Ptr to dump destination.
/// @param[in] n        dumpto entries.
/// @return  Dumped count.
int memmgr::nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	const memmgr* const mm = get_memmgr();

	const alloc_entry* ent = mm->nofree_chain.get_head();
	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = mm->nofree_chain.get_next(ent);
	}

	return i;
}

void* memmgr::memmgr_alloc(uptr size, uptr align)
{
	memmgr* const mm = get_memmgr();

	for (alloc_entry* ent = mm->free_chain.get_head();
	     ent;
	     ent = mm->free_chain.get_next(ent)) {
		const u64 align_gap = up_align(ent->head, align) - ent->head;
		if ((ent->bytes - align_gap) < size)
			continue;

		u64 r;
		if (ent->bytes == size) {
			r = ent->head;
			mm->free_chain.remove(ent);
			mm->nofree_chain.insert_head(ent);
		} else {
			alloc_entry* newent = mm->new_entry();
			if (newent == 0)
				continue;

			if (align_gap != 0) {
				alloc_entry* newent2 = mm->new_entry();
				if (newent2 == 0) {
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

	return 0;
}

void memmgr::memmgr_free(void* p)
{
	// 割り当て済みリストから p を探す。

	u64 head = reinterpret_cast<u64>(p);
	alloc_entry* ent;
	for (ent = nofree_chain.get_head();
	     ent;
	     ent = nofree_chain.get_next(ent))
	{
		if (ent->head == head) {
			nofree_chain.remove(ent);
			break;
		}
	}

	if (!ent)
		return;

	// 空きリストから p の直後のアドレスを探す。
	// p と p の直後のアドレスを結合して ent とする。

	u64 tail = head + ent->bytes;
	for (alloc_entry* ent2 = free_chain.get_head();
	     ent2;
	     ent2 = free_chain.get_next(ent2))
	{
		if (ent2->head == tail) {
			ent->bytes += ent2->bytes;
			free_chain.remove(ent2);
			ent2->unset();
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	head = ent->head;
	for (alloc_entry* ent2 = free_chain.get_head();
	     ent2;
	     ent2 = free_chain.get_next(ent2))
	{
		if ((ent2->head + ent2->bytes) == head) {
			ent2->bytes += ent->bytes;
			ent->unset();
			ent = 0;
			break;
		}
	}

	if (ent)
		free_chain.insert_head(ent);
}

}  // namespace


/**
 * @brief  Initialize memmgr.
 */
void memmgr_init()
{
	//memmgr* mm = new (get_memmgr()) memmgr;
	memmgr* mm = new (reinterpret_cast<void*>(memmgr_buf_)) memmgr;
	memmgr_ptr = mm;

	mm->init();

	mm->import();

	// 先頭の固定利用アドレスを予約領域とする。
	// memmgr が NULL アドレスのメモリを確保することを防ぐ意味もある。
	mm->reserve_range(0x00000000, SETUP_MEMMGR_RESERVED_PADR);
}

/// @brief  Allocate memory.
/// @param[in] size Memory bytes.
/// @param[in] align Memory aliments. Must to be 2^n.
///     If align == 256, then return address is "0x....00".
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns NULL.
void* memmgr_alloc(uptr size, uptr align)
{
	memmgr* const mm = get_memmgr();

	return mm->memmgr_alloc(size, align);
}

/// @brief  Free memory.
/// @param p Ptr to memory frees.
void memmgr_free(void* p)
{
	return get_memmgr()->memmgr_free(p);
}

int memmgr_freemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	return get_memmgr()->freemem_dump(dumpto, n);
}

int memmgr_nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	return get_memmgr()->nofreemem_dump(dumpto, n);
}
