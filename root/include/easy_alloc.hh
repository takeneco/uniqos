/// @file   easy_alloc.hh
/// @brief  Easy memory allocation implement.

//  uniqos  --  Unique Operating System
//  (C) 2011 KATO Takeshi
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

#ifndef INCLUDE_EASY_ALLOC_HH_
#define INCLUDE_EASY_ALLOC_HH_

#include "arch.hh"
#include "chain.hh"
#include "log.hh"


/// @brief easy_alloc を使用後にメモリの状態を列挙するときの列挙子。
struct easy_alloc_enum
{
	bool avoid;
	const void* entry;
};

/// @brief  簡単なメモリ管理の実装
//
/// 使い方
/// -# 最初に add_free() で割り当て可能な空きメモリを追加する。
///    add_free() で追加したメモリの中に割り当てたくないメモリが
///    含まれる場合は reserve() で割り当てを禁止する。
/// -# 割り当て可能なメモリの設定が終わったら、mem_alloc() でメモリを割り当て、
///    mem_free() で開放できる。
/// -# 最後に alloc_info() / avoid_alloc_info() で確保状態のメモリを列挙し、
///    次のフェーズへ引き継ぐ。
//
/// - このクラスは作業メモリが不足してエラーを返すことがある。
///   その場合はクラスのオブジェクトを破壊した可能性があり使い続けられない。
/// - 作業メモリ不足になる場合は BUF_COUNT を増やすしかない。
template<int BUF_COUNT>
class easy_alloc
{
public:
	easy_alloc() {}

	void init();

	bool add_free(uptr adr, uptr bytes, bool avoid);
	bool reserve(uptr adr, uptr bytes, bool forget);
	void* alloc(uptr size, uptr align, bool forget);
	bool free(void* p);

private:
	struct entry
	{
		uptr adr;
		uptr bytes;  ///< unused if bytes == 0.

		bichain_node<entry> chain_node_;
		bichain_node<entry>& chain_hook() { return chain_node_; }

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

	entry entry_buf[BUF_COUNT];

private:
	entry* new_entry(uptr adr, uptr bytes);
	void free_entry(entry* e);
	bool _reserve_range(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    uptr r_head, uptr r_tail, bool forget);
	entry* _alloc(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    uptr size, uptr align, bool forget);
	bool _free(
	    entry_chain& _free_chain, entry_chain& _alloc_chain,
	    void* p);

public:
	/// @fn const void* alloc_info() const
	/// @fn const void* alloc_info_next(const void* e, uptr* adr, uptr* bytes) const
	/// @fn const void* avoid_alloc_info() const
	/// @fn const void* avoid_alloc_info_next(const void* e, uptr* adr, uptr* bytes) const
	/// @brief reserve() か alloc() で空き状態ではなくなったメモリを
	///        列挙する。
	//
	/// alloc_info() で先頭ポインタを取って、alloc_info_next() で列挙する。
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

	/// @fn const void* free_info() const
	/// @fn const void* free_info_next(const void* e, uptr* adr, uptr* bytes) const
	/// @fn const void* avoid_free_info() const
	/// @fn const void* avoid_free_info_next(const void* e, uptr* adr, uptr* bytes) const
	/// @brief 空き状態のメモリを列挙する。
	//
	/// free_info() で先頭ポインタを取って、free_info_next() で列挙する。
	const void* free_info() const {
		return free_chain.head();
	}
	const void* free_info_next(
	    const void* e, uptr* adr, uptr* bytes) const {
		const entry* _e = reinterpret_cast<const entry*>(e);
		*adr = _e->adr;
		*bytes = _e->bytes;
		return free_chain.next(_e);
	}
	const void* avoid_free_info() const {
		return av_free_chain.head();
	}
	const void* avoid_free_info_next(
	    const void* e, uptr* adr, uptr* bytes) const {
		const entry* _e = reinterpret_cast<const entry*>(e);
		*adr = _e->adr;
		*bytes = _e->bytes;
		return av_free_chain.next(_e);
	}

	void all_alloc_info(easy_alloc_enum* x) const;
	bool all_alloc_info_next(
	    easy_alloc_enum* x, uptr* adr, uptr* bytes) const;
	void all_free_info(easy_alloc_enum* x) const;
	bool all_free_info_next(
	    easy_alloc_enum* x, uptr* adr, uptr* bytes) const;

	void debug_dump(kernel_log& log) {
		for (entry* e=free_chain.head();e;e=free_chain.next(e))
			log("  free:").u(e->adr,16)(" +").u(e->bytes,16)();
		for (entry* e=av_free_chain.head();e;e=av_free_chain.next(e))
			log("avfree:").u(e->adr,16)(" +").u(e->bytes,16)();
		for (entry* e=alloc_chain.head();e;e=alloc_chain.next(e))
			log("  aloc:").u(e->adr,16)(" +").u(e->bytes,16)();
		for (entry* e=av_alloc_chain.head();e;e=av_alloc_chain.next(e))
			log("avaloc:").u(e->adr,16)(" +").u(e->bytes,16)();
	}
};

/// @brief  Initialize working area.
template<int BUF_COUNT>
void easy_alloc<BUF_COUNT>::init()
{
	for (int i = 0; i < BUF_COUNT; i++)
		entry_buf[i].unset();
}

/// @brief  Add memory to free_chain.
/// @param[in] avoid  なるべく空けておきたいメモリなら true。
/// @retval true  Succeeds.
/// @retval false No enough working memory.
template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::add_free(uptr adr, uptr bytes, bool avoid)
{
	if (bytes == 0)
		return true;

	entry* ent = new_entry(adr, bytes);
	if (ent == 0)
		return false;

	(avoid ? av_free_chain : free_chain).insert_head(ent);

	return true;
}

/// @brief 指定したメモリ範囲をメモリ確保の対象外にする。
/// @param[in] adr    取り除くメモリの先頭アドレス。
/// @param[in] bytes  取り除くメモリのバイト数。
/// @param[in] forget true ならばメモリ確保の対象外にしたことを忘れる。
/// @retval true  Succeeds.
/// @retval false No enough working memory. The easy_alloc object is broken.
//
/// - メモリ確保の対象外にしたい領域がある場合は、メモリを確保する前に
///   この関数で設定しなければならない。
/// - forget の扱いは mem_alloc() の forget と同じで、割り当て可能なメモリが
///   あったこと自体を忘れてしまう。
/// - forget = false ならば、対象外にしたメモリ範囲を
///   alloc_info() / avoid_alloc_info() で拾うことができる。
template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::reserve(uptr adr, uptr bytes, bool forget)
{
	bool r = _reserve_range(
	    free_chain, alloc_chain, adr, bytes, forget);
	r &= _reserve_range(
	    av_free_chain, av_alloc_chain, adr, bytes, forget);

	return r;
}

/// @brief  Allocate memory.
/// @param[in] bytes  required memory size.
/// @param[in] align  Memory aliment. Must to be 2^n.
///     EX: If align == 256, then return address is "0x....00".
/// @param[in] forget Forget this memory.
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns 0.
//
/// - forget = true とすると、このメモリを割り当てたこと自体を忘れてしまう。
/// - mem_free() で開放するときは forget = false としなければならない。
/// - mem_free() で開放しない場合でも、割り当てたことを覚えておいて、
///   後で alloc_info() で知りたいときは forget = false としなければならない。
/// - forget = true とすると、add_free() で avoid = true を指定したメモリを
///   優先的に割り当てる。
///   forget = true ならばメモリ管理の終了とともにメモリが開放されるという
///   考え方なので、avoid のメモリを避ける必要はないということ。
template<int BUF_COUNT>
void* easy_alloc<BUF_COUNT>::alloc(uptr bytes, uptr align, bool forget)
{
	entry* ent;

	if (forget) {
		ent = _alloc(av_free_chain, av_alloc_chain,
		                 bytes, align, forget);

		if (!ent)
			ent = _alloc(free_chain, alloc_chain,
			                 bytes, align, forget);
	} else {
		ent = _alloc(free_chain, alloc_chain,
		                 bytes, align, forget);

		if (!ent)
			ent = _alloc(av_free_chain, av_alloc_chain,
			                 bytes, align, forget);
	}

	return reinterpret_cast<void*>(ent->adr);
}

/// @brief  Free memory.
/// @param[in] p  Ptr to memory free.
template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::free(void* p)
{
	bool r = _free(free_chain, alloc_chain, p);

	if (!r)
		r = _free(av_free_chain, av_alloc_chain, p);

	return true;
}

template<int BUF_COUNT>
void easy_alloc<BUF_COUNT>::all_alloc_info(easy_alloc_enum* x) const
{
	x->avoid = false;
	x->entry = alloc_info();
}

template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::all_alloc_info_next(
    easy_alloc_enum* x, uptr* adr, uptr* bytes) const
{
	const void* next;

	if (x->avoid == false) {
		if (x->entry == 0) {
			x->avoid = true;
			x->entry = avoid_alloc_info();
			return all_alloc_info_next(x, adr, bytes);
		}
		next = alloc_info_next(x->entry, adr, bytes);
	}
	else {
		if (x->entry == 0)
			return false;
		next = avoid_alloc_info_next(x->entry, adr, bytes);
	}

	x->entry = next;

	return true;
}

template<int BUF_COUNT>
void easy_alloc<BUF_COUNT>::all_free_info(easy_alloc_enum* x) const
{
	x->avoid = false;
	x->entry = free_info();
}

template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::all_free_info_next(
    easy_alloc_enum* x, uptr* adr, uptr* bytes) const
{
	const void* next;

	if (x->avoid == false) {
		if (x->entry == 0) {
			x->avoid = true;
			x->entry = avoid_free_info();
			return all_free_info_next(x, adr, bytes);
		}
		next = free_info_next(x->entry, adr, bytes);
	}
	else {
		if (x->entry == 0)
			return false;
		next = avoid_free_info_next(x->entry, adr, bytes);
	}

	x->entry = next;

	return true;
}

/// @brief  Search unused entry from entry_buf[].
/// @return  If unused entry found, this func returns ptr to unused entry.
/// @return  If unused entry not found, this func returns 0.
template<int BUF_COUNT>
typename easy_alloc<BUF_COUNT>::entry*
easy_alloc<BUF_COUNT>::new_entry(uptr adr, uptr bytes)
{
	entry* const buf = entry_buf;

	for (int i = 0; i < BUF_COUNT; ++i) {
		if (buf[i].bytes == 0) {
			//buf[i].bytes = 1;  // means using.
			buf[i].set(adr, bytes);
			return &buf[i];
		}
	}

	return 0;
}

template<int BUF_COUNT>
void easy_alloc<BUF_COUNT>::free_entry(entry* e)
{
	if (e)
		//e->bytes = 0;  // means free.
		e->unset();
}

/// @brief メモリ範囲を空きメモリチェイン(free_chain)から取り除く。
/// @param[in] _free_chain   空きメモリチェイン。
/// @param[in] _alloc_chain  使用メモリチェイン。
/// @param[in] adr    取り除くメモリの先頭アドレス。
/// @param[in] bytes  取り除くメモリの長さ。
/// @retval true  Succeeds.
/// @retval false No enough working memory.
///               The easy_alloc object might be broken.
//
/// - forget = false ならば _free_chain から取り除いたメモリは
///   _alloc_chain に格納する。
/// - forget = true ならば _free_chain からメモリを取り除くだけ。
///   _alloc_chain は無視する。
template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::_reserve_range(
    entry_chain& _free_chain,
    entry_chain& _alloc_chain,
    uptr adr,
    uptr bytes,
    bool forget)
{
	uptr r_head = adr;
	uptr r_tail = adr + bytes - 1;

	for (entry* ent = _free_chain.head(); ent; )
	{
		uptr e_head = ent->adr;
		uptr e_tail = e_head + ent->bytes - 1;

		if (e_head < r_head && r_tail < e_tail) {
			if (forget == false) {
				entry* alloc = new_entry(adr, bytes);
				if (!alloc)
					return false;
				_alloc_chain.insert_head(alloc);
			}

			entry* free2 = new_entry(r_tail + 1, e_tail - r_tail);
			if (!free2)
				return false;
			_free_chain.insert_head(free2);

			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head <= r_tail) {
			if (forget == false) {
				entry* alloc = new_entry(
				    e_head, min(r_tail, e_tail) - e_head + 1);
				if (!alloc)
					return false;
				_alloc_chain.insert_head(alloc);
			}

			e_head = r_tail + 1;
		}

		if (r_head <= e_tail && e_tail <= r_tail) {
			if (forget == false) {
				const uptr h = max(r_head, e_head);
				entry* alloc = new_entry(h, e_tail - h + 1);
				if (!alloc)
					return false;
				_alloc_chain.insert_head(alloc);
			}

			e_tail = r_head - 1;
		}

		if (e_head > e_tail) {
			entry* next_ent = _free_chain.next(ent);
			_free_chain.remove(ent);
			free_entry(ent);
			ent = next_ent;
		} else {
			ent->set(e_head, e_tail - e_head + 1);
			ent = _free_chain.next(ent);
		}
	}

	return true;
}

/// @brief  空きメモリを割り当てる。
/// @param[in] _free_chain  free_chain か av_free_chain。
/// @param[in] _alloc_chain alloc_chain か av_alloc_chain。
/// @param[in] size         メモリサイズ。
/// @param[in] align        アライメント。
//
/// - forget = false ならば _free_chain から割り当てたメモリは
///   _alloc_chain に格納する。
/// - forget = true ならば _free_chain からメモリを割り当てるだけ。
///   _alloc_chain は無視する。
template<int BUF_COUNT>
typename easy_alloc<BUF_COUNT>::entry*
easy_alloc<BUF_COUNT>::_alloc(
    entry_chain& _free_chain,
    entry_chain& _alloc_chain,
    uptr size, uptr align, bool forget)
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
		if (forget == false)
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

		if (forget == false)
			_alloc_chain.insert_head(r);

		ent->adr += size;
		ent->bytes -= size;
	}

	return r;
}

template<int BUF_COUNT>
bool easy_alloc<BUF_COUNT>::_free(
    entry_chain& _free_chain,
    entry_chain& _alloc_chain,
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


#endif  // include guard
