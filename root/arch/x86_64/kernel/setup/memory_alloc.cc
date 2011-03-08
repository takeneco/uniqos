/// @file   memory_alloc.cc
/// @brief  Easy memory allocation implement using setup.
//
// (C) 2010-2011 KATO Takeshi
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

class alloc
{
	struct entry
	{
		u64 head;
		u64 bytes;  ///< unused if bytes == 0.

		bichain_link<entry> chain_link_;
		bichain_link<entry>& chain_hook() { return chain_link_; }

		void set(u64 head_, u64 bytes_) {
			head = head_;
			bytes = bytes_;
		}
		void unset() {
			bytes = 0;
		}
	};

	typedef bichain<entry, &entry::chain_hook> entry_chain;

	/// free memory list
	entry_chain free_chain;

	/// reserved or alloced memory list
	entry_chain nofree_chain;

	enum { ENTRY_BUF_COUNT = 256 };
	entry entry_buf[ENTRY_BUF_COUNT];

private:
	entry* new_entry();
	void add_free_entry(const acpi_memmap* raw);

public:
	alloc() : free_chain(), nofree_chain() {}

	void init();
	void import();
	void reserve_range(u64 r_head, u64 r_tail);
	int freemem_dump(setup_memmgr_dumpdata* dumpto, int n);
	int nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n);
	void* memory_alloc(uptr size, uptr align);
	void memory_free(void* p);
};

// コンストラクタを呼ばせない。
uptr alloc_buf_[sizeof (alloc) / sizeof (uptr)];
inline alloc* get_alloc() { return reinterpret_cast<alloc*>(alloc_buf_); }

/// @brief  Search unused entry from entry_buf[].
/// @return  If unused entry found, this func returns ptr to unused entry.
/// @return  If unused entry not found, this func returns 0.
alloc::entry* alloc::new_entry()
{
	entry* const buf = entry_buf;

	for (int i = 0; i < ENTRY_BUF_COUNT; ++i) {
		if (buf[i].bytes == 0) {
			buf[i].bytes = 1;  // means using.
			return &buf[i];
		}
	}

	return 0;
}

/// @brief  Add free memory info to free_chain from acpi_memmap.
void alloc::add_free_entry(const acpi_memmap* raw)
{
	if (raw->length == 0)
		return;

	entry* ent = new_entry();
	if (ent == 0)
		return;

	ent->set(raw->base, raw->length);

	free_chain.insert_head(ent);
}

/// @brief  Initialize work area.
void alloc::init()
{
	for (int i = 0; i < ENTRY_BUF_COUNT; i++)
		entry_buf[i].unset();
}

/// @brief  Import memory map by ACPI in real mode.
void alloc::import()
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

/// @brief メモリ空間を free_chain から取り除く。
/// @param[in] r_head 取り除くメモリの先頭アドレス。
/// @param[in] r_tail 取り除くメモリの終端アドレス。
void alloc::reserve_range(u64 r_head, u64 r_tail)
{
	r_tail += 1;

	for (entry* ent = free_chain.get_head();
	     ent;
	     ent = free_chain.get_next(ent))
	{
		uptr e_head = ent->head;
		uptr e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			entry* ent2 = new_entry();
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
int alloc::freemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	const entry* ent = free_chain.get_head();

	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = free_chain.get_next(ent);
	}

	return i;
}

/// @brief  使用メモリ情報をカーネルに渡すためにメモリ上へ出力する。
/// @param[out] dumptp  Ptr to dump destination.
/// @param[in] n        dumpto entries.
/// @return  Dumped count.
int alloc::nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	const entry* ent = nofree_chain.get_head();

	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = nofree_chain.get_next(ent);
	}

	return i;
}

void* alloc::memory_alloc(uptr size, uptr align)
{
	for (entry* ent = free_chain.get_head();
	     ent;
	     ent = free_chain.get_next(ent))
	{
		const u64 align_gap = up_align(ent->head, align) - ent->head;
		if ((ent->bytes - align_gap) < size)
			continue;

		u64 r;
		if (ent->bytes == size) {
			r = ent->head;
			free_chain.remove(ent);
			nofree_chain.insert_head(ent);
		} else {
			entry* newent = new_entry();
			if (newent == 0)
				continue;

			if (align_gap != 0) {
				entry* newent2 = new_entry();
				if (newent2 == 0) {
					newent->unset();
					continue;
				}
				newent2->set(ent->head, align_gap);
				free_chain.insert_head(newent2);

				ent->head += align_gap;
				ent->bytes -= align_gap;
			}

			r = ent->head;
			newent->set(ent->head, size);
			nofree_chain.insert_head(newent);

			ent->head += size;
			ent->bytes -= size;
		}

		return reinterpret_cast<void*>(r);
	}

	return 0;
}

void alloc::memory_free(void* p)
{
	// 割り当て済みリストから p を探す。

	u64 head = reinterpret_cast<u64>(p);
	entry* ent;
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
	for (entry* ent2 = free_chain.get_head();
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
	for (entry* ent2 = free_chain.get_head();
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
 * @brief  Initialize alloc.
 */
void memmgr_init()
{
	alloc* mm = new (get_alloc()) alloc;

	mm->init();

	mm->import();

	// 先頭の固定利用アドレスを予約領域とする。
	// alloc が NULL アドレスのメモリを確保することを防ぐ意味もある。
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
	return get_alloc()->memory_alloc(size, align);
}

/// @brief  Free memory.
/// @param p Ptr to memory frees.
void memmgr_free(void* p)
{
	return get_alloc()->memory_free(p);
}

int memmgr_freemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	return get_alloc()->freemem_dump(dumpto, n);
}

int memmgr_nofreemem_dump(setup_memmgr_dumpdata* dumpto, int n)
{
	return get_alloc()->nofreemem_dump(dumpto, n);
}
