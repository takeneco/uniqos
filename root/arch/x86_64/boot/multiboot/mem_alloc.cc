/// @file   mem_alloc.cc
/// @brief  Easy memory allocation implement.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch.hh"
#include "chain.hh"
#include "misc.hh"
#include "placement_new.hh"
#include "log.hh"


namespace {

class mem
{
public:
	mem() {}

	void init();

	void add_free(uptr adr, uptr bytes, bool avoid);

	void* mem_alloc(uptr size, uptr align, bool kern_forget);
	void mem_free(void* p);

	bool reserve(uptr adr, uptr bytes);

private:
	struct entry
	{
		uptr adr;
		uptr bytes;  ///< unused if bytes == 0.

		bichain_link<entry> chain_link_;
		bichain_link<entry>& chain_hook() { return chain_link_; }

		void set(uptr _adr, uptr _bytes) {
			adr = _adr;
			bytes = _bytes;
		}
		void unset() {
			bytes = 0;
		}
	};

	typedef bichain<entry, &entry::chain_hook> entry_chain;

	/// free memory list
	entry_chain free_chain;

	/// free memory, but low priority (avoid)
	entry_chain av_free_chain;

	/// reserved or alloced memory list
	entry_chain alloc_chain;
	entry_chain av_alloc_chain;

	enum { ENTRY_BUF_COUNT = 256 };
	entry entry_buf[ENTRY_BUF_COUNT];

	/// カーネルに jmp したときに不要になるメモリ。
	/// カーネル側で開放できるように、情報をカーネルに渡す必要がある。
	enum { KERNEL_FORGET_COUNT = ENTRY_BUF_COUNT / 2 };
	u16 kernel_forget[KERNEL_FORGET_COUNT];  /// indices of entry_buf
	u32 kernel_forget_count;

private:
	entry* new_entry(uptr adr, uptr bytes);
	void free_entry(entry* e);
	bool _reserve_range(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    uptr r_head, uptr r_tail);

public:
	const void* alloc_info() const {
		return alloc_chain.head();
	}
	const void* alloc_info_next(
	    const void* e, uptr* adr, uptr* bytes) const {
		const entry* _e = reinterpret_cast<const entry*>(e);
		*adr = _e->adr;
		*bytes = _e->bytes;
		return alloc_chain.next(_e);
	}
	const void* avoid_alloc_info() const {
		return av_alloc_chain.head();
	}
	const void* avoid_alloc_info_next(
	    const void* e, uptr* adr, uptr* bytes) const {
		const entry* _e = reinterpret_cast<const entry*>(e);
		*adr = _e->adr;
		*bytes = _e->bytes;
		return av_alloc_chain.next(_e);
	}


//	int freemem_dump(setup_memory_dumpdata* dumpto, int n);
//	int nofreemem_dump(setup_memory_dumpdata* dumpto, int n);
	entry* _mem_alloc(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    uptr size, uptr align);
	bool _mem_free(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    void* p);
};

/// @brief  Initialize working area.
void mem::init()
{
	for (int i = 0; i < ENTRY_BUF_COUNT; i++)
		entry_buf[i].unset();

	kernel_forget_count = 0;
}

/// @brief  Add memory to free_chain.
/// @param[in] avoid  カーネルのために空けておきたいメモリなら true。
void mem::add_free(uptr adr, uptr bytes, bool avoid)
{
	if (bytes == 0)
		return;

	entry* ent = new_entry(adr, bytes);
	if (ent == 0)
		return;

	(avoid ? av_free_chain : free_chain).insert_head(ent);
}

/// @brief  Allocate memory.
/// @param[in] bytes  required memory size.
/// @param[in] align  memory alignment.
/// @param[in] kern_forget true ならkernelに jmp した直後にkernelが開放する。
///                        true なら avoid の領域を割り当てる。
///  - カーネルに jmp した後も使い続けるなら kern_forget = false
///  - カーネルに jmp する前に明示的に開放するなら kern_forget = false
void* mem::mem_alloc(uptr bytes, uptr align, bool kern_forget)
{
	entry* ent;

	if (kern_forget) {
		ent = _mem_alloc(
		    av_free_chain, av_alloc_chain, bytes, align);

		if (!ent)
			ent = _mem_alloc(
			    free_chain, alloc_chain, bytes, align);

		const u16 i = ent - entry_buf;
		kernel_forget[kernel_forget_count++] = i;
	} else {
		ent = _mem_alloc(free_chain, alloc_chain, bytes, align);

		if (!ent)
			ent = _mem_alloc(
			    av_free_chain, av_alloc_chain, bytes, align);
	}

	return reinterpret_cast<void*>(ent->adr);
}

/// @brief  Free memory.
void mem::mem_free(void* p)
{
	bool r = _mem_free(free_chain, alloc_chain, p);

	if (!r)
		r = _mem_free(av_free_chain, av_alloc_chain, p);

	if (!r)
		log()("mem_free(): unknown address passed. (")(p)(")")();
}

/// @brief 指定したメモリ範囲をメモリ確保の対象外にする。
/// @param[in] adr   取り除くメモリの先頭アドレス。
/// @param[in] bytes 取り除くメモリのバイト数。
/// @param[in] forget true ならばメモリ確保の対象外にしたこと自体を忘れる。
/// @retval true  Succeeds.
/// @retval false No enough working memory.
//
/// - メモリ確保の対象外にしたい領域がある場合は、メモリを確保する前に
///   この関数で設定しなければならない。
/// - forget = true のときは kernel に jmp した後で確保可能になる。
bool mem::reserve(uptr adr, uptr bytes)
{
	bool r = _reserve_range(
	    free_chain, alloc_chain, adr, bytes);
	r &= _reserve_range(
	    av_free_chain, av_alloc_chain, adr, bytes);

	return r;
}

/// @brief  Search unused entry from entry_buf[].
/// @return  If unused entry found, this func returns ptr to unused entry.
/// @return  If unused entry not found, this func returns 0.
mem::entry* mem::new_entry(uptr adr, uptr bytes)
{
	entry* const buf = entry_buf;

	for (int i = 0; i < ENTRY_BUF_COUNT; ++i) {
		if (buf[i].bytes == 0) {
			//buf[i].bytes = 1;  // means using.
			buf[i].set(adr, bytes);
			return &buf[i];
		}
	}

	return 0;
}

void mem::free_entry(entry* e)
{
	if (e)
		//e->bytes = 0;  // means free.
		e->unset();
}

/// @brief メモリ範囲を空きメモリチェイン(free_chain)から取り除く。
/// @param[in] _free_chain   空きメモリチェイン。
/// @param[in] _alloc_chain 使用メモリチェイン。
/// @param[in] r_head 取り除くメモリの先頭アドレス。
/// @param[in] r_tail 取り除くメモリの終端アドレス。
//
/// _free_chain から取り除いたメモリは _alloc_chain に格納する。
bool mem::_reserve_range(
    entry_chain& _free_chain, entry_chain& _alloc_chain,
    uptr adr, uptr bytes)
{
	uptr r_head = adr;
	uptr r_tail = adr + bytes;

	for (entry* ent = _free_chain.head();
	     ent;
	     ent = _free_chain.next(ent))
	{
		uptr e_head = ent->adr;
		uptr e_tail = e_head + ent->bytes;

		if (e_head < r_head && r_tail < e_tail) {
			entry* alloc = new_entry(adr, bytes);
			entry* free2 = new_entry(r_tail, e_tail - r_tail);
			if (!alloc || !free2) {
				free_entry(alloc);
				free_entry(free2);
				return false;
			}

			_alloc_chain.insert_head(alloc);
			_free_chain.insert_head(free2);
			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head < r_tail) {
			entry* alloc = new_entry(
			    e_head, min(r_tail, e_tail) - e_head);
			if (!alloc)
				return false;
			_alloc_chain.insert_head(alloc);

			e_head = r_tail;
		}

		if (r_head <= e_tail && e_tail < r_tail) {
			const uptr h = max(r_head, e_head);
			entry* alloc = new_entry(h, e_tail - h);
			if (!alloc)
				return false;
			_alloc_chain.insert_head(alloc);

			e_tail = r_head;
		}

		if (e_head >= e_tail) {
			entry* next_ent = _free_chain.next(ent);
			_free_chain.remove(ent);
			free_entry(ent);
			ent = next_ent;
		} else {
			ent->set(e_head, e_tail - e_head);
		}
	}

	return true;
}
/*
/// @brief  空きメモリ情報をカーネルに渡すためにメモリ上へ出力する。
/// @param[out] dumpto  Ptr to dump destination.
/// @param[in] n        dumpto entries.
/// @return  Dumped count.
int mem::freemem_dump(setup_memory_dumpdata* dumpto, int n)
{
	const entry* ent = free_chain.head();

	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = free_chain.next(ent);
	}

	return i;
}

/// @brief  使用メモリ情報をカーネルに渡すためにメモリ上へ出力する。
/// @param[out] dumptp  Ptr to dump destination.
/// @param[in] n        dumpto entries.
/// @return  Dumped count.
int mem::nofreemem_dump(setup_memory_dumpdata* dumpto, int n)
{
	const entry* ent = nofree_chain.head();

	int i;
	for (i = 0; ent && i < n; i++) {
		dumpto[i].head = ent->head;
		dumpto[i].bytes = ent->bytes;
		ent = nofree_chain.next(ent);
	}

	return i;
}
*/

mem::entry* mem::_mem_alloc(
    entry_chain& _free_chain, entry_chain& _alloc_chain,
    uptr size, uptr align)
{
	uptr align_gap;
	entry* ent;
	for (ent = _free_chain.head(); ent; ent = _free_chain.next(ent))
	{
		align_gap = up_align(ent->adr, align) - ent->adr;
		if ((ent->bytes - align_gap) >= size)
			break;
	}
	if (!ent)
		return 0;

	entry* r;
	if (ent->bytes == size) {
		r = ent;
		_free_chain.remove(ent);
		_alloc_chain.insert_head(ent);
	} else {
		if (align_gap != 0) {
			entry* newent = new_entry(ent->adr, align_gap);
			if (!newent)
				return 0;

			_free_chain.insert_head(newent);

			ent->adr += align_gap;
			ent->bytes -= align_gap;
		}
		r = new_entry(ent->adr, size);
		if (!r)
			return 0;

		_alloc_chain.insert_head(r);

		ent->adr += size;
		ent->bytes -= size;
	}

	return r;
}

bool mem::_mem_free(
    entry_chain& _free_chain, entry_chain& _alloc_chain,
    void* p)
{
	// 割り当て済みリストから p を探す。

	uptr adr = reinterpret_cast<uptr>(p);
	entry* ent;
	for (ent = _alloc_chain.head(); ent; ent = _alloc_chain.next(ent)) {
		if (ent->adr == adr) {
			_alloc_chain.remove(ent);
			break;
		}
	}

	if (!ent)
		return false;

#ifdef DEBUG_BOOT
	const u16 n = ent - entry_buf;
	for (u32 i = 0; i < kernel_forget_count; ++i) {
		if (n == kernel_forget[i])
			log()(__FILE__, __LINE__, __func__)
			    (":Fatail: abnomal memory free.")();
	}
#endif  // DEBUG_BOOT

	// 空きリストから p の直後のアドレスを探す。
	// p と p の直後のアドレスを結合して ent とする。

	uptr tail = adr + ent->bytes;
	for (entry* ent2 = _free_chain.head();
	     ent2;
	     ent2 = _free_chain.next(ent2))
	{
		if (ent2->adr == tail) {
			ent->bytes += ent2->bytes;
			_free_chain.remove(ent2);
			ent2->unset();
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	adr = ent->adr;
	for (entry* ent2 = _free_chain.head();
	     ent2;
	     ent2 = _free_chain.next(ent2))
	{
		if ((ent2->adr + ent2->bytes) == adr) {
			ent2->bytes += ent->bytes;
			ent->unset();
			ent = 0;
			break;
		}
	}

	if (ent)
		_free_chain.insert_head(ent);

	return true;
}

// コンストラクタを呼ばせない。
uptr mem_buf_[sizeof (mem) / sizeof (uptr)];
inline mem* get_mem() {
	return reinterpret_cast<mem*>(mem_buf_);
}

}  // namespace


/**
 * @brief  Initialize alloc.
 */
void mem_init()
{
	new (get_mem()) mem;
	get_mem()->init();
}

void mem_add(uptr adr, uptr bytes, bool avoid)
{
	get_mem()->add_free(adr, bytes, avoid);
}

/// @brief  Allocate memory.
/// @param[in] size Memory bytes.
/// @param[in] align Memory aliments. Must to be 2^n.
///     If align == 256, then return address is "0x....00".
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns NULL.
void* mem_alloc(uptr size, uptr align, bool kern_forget)
{
	return get_mem()->mem_alloc(size, align, kern_forget);
}

/// @brief  Free memory.
/// @param p Ptr to memory frees.
void mem_free(void* p)
{
	get_mem()->mem_free(p);
}

bool mem_reserve(uptr adr, uptr bytes)
{
	return get_mem()->reserve(adr, bytes);
}

void mem_alloc_info(mem_enum* me)
{
	me->avoid = false;
	me->entry = get_mem()->alloc_info();
}

bool mem_alloc_info_next(mem_enum* me, uptr* adr, uptr* bytes)
{
	const void* next = get_mem()->alloc_info_next(me->entry, adr, bytes);

	if (!next && me->avoid == false) {
		next = get_mem()->avoid_alloc_info();
		me->avoid = true;
	}
	me->entry = next;

	return next != 0;
}

/*
int freemem_dump(setup_memory_dumpdata* dumpto, int n)
{
	return get_mem()->freemem_dump(dumpto, n);
}

int nofreemem_dump(setup_memory_dumpdata* dumpto, int n)
{
	return get_mem()->nofreemem_dump(dumpto, n);
}
*/

namespace arch {
namespace page {

cause::stype alloc(TYPE page_type, uptr* padr)
{
	const u32 size = inline_page_size(page_type);
	void* p = mem_alloc(size, size, false);
	if (!p)
		return cause::NO_MEMORY;

	*padr = reinterpret_cast<uptr>(p);
	return cause::OK;
}

cause::stype free(TYPE, uptr padr)
{
	mem_free(reinterpret_cast<void*>(padr));
	return cause::OK;
}

}  // namespace page
}  // namespace arch

