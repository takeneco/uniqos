/// @file   cheap_alloc.hh
/// @brief  cheap address allocation implement.

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

#ifndef INCLUDE_CHEAP_ALLOC_HH_
#define INCLUDE_CHEAP_ALLOC_HH_

#include "arch.hh"
#include "chain.hh"
#include "log.hh"


/// @brief  簡単なメモリ管理の実装
/// @tparam BUF_COUNT 内部バッファのエントリ数。
//
/// 使い方
/// -# 最初に add_free() で割り当て可能な空きメモリを追加する。
///    add_free() で追加したメモリの中に割り当てたくないメモリが
///    含まれる場合は reserve() で割り当てを禁止する。
/// -# 割り当て可能なメモリの設定が終わったら、alloc() でメモリを割り当て、
///    free() で開放できる。
/// -# 最後に enum_xxx() / enum_xxx_next() で確保状態のメモリを列挙し、
///    次のフェーズへ引き継ぐ。
///
/// slot について
/// - SLOT_COUNT(=8)個のスロットがあり、それぞれを別のメモリ領域として扱う。
/// - add_free() は、どのスロットに対する操作かを 0 から始まる
///   インデックスで指定する。
/// - reserve() / alloc() / free() / enum_xxx() は対象スロットを
///   ビットマスク (1 << index) で指定する。
///
/// - このクラスは作業メモリが不足してエラー(false)を返すことがある。
///   その場合はクラスのオブジェクトを破壊した可能性があり使い続けられない。
/// - 作業メモリ不足にならないように、大きめの BUF_COUNT を指定するしかない。
template<int BUF_COUNT>
class cheap_alloc
{
	struct range;

public:
	typedef u8 slot_index;
	typedef u8 slot_mask;

	enum { SLOT_COUNT = 8 };

	/// Must be call for initialize.
	cheap_alloc() {}

	bool add_free(slot_index slot, uptr adr, uptr bytes);
	bool reserve(slot_mask slotm, uptr adr, uptr bytes, bool forget);
	void* alloc(slot_mask slotm, uptr bytes, uptr align, bool forget);
	bool free(slot_mask slotm, void* p);

	/// @brief cheap_alloc を使用後にメモリの状態を列挙するときの列挙子。
	struct enum_desc
	{
		slot_mask slotm;
		slot_index index;
		const range* range_node;
	};
	void enum_free(slot_mask slotm, enum_desc* x) const;
	bool enum_free_next(enum_desc* x, uptr* adr, uptr* bytes) const;
	void enum_alloc(slot_mask slotm, enum_desc* x) const;
	bool enum_alloc_next(enum_desc* x, uptr* adr, uptr* bytes) const;

private:
	struct range
	{
		range() : bytes(0), _chain_node() {}

		void set(uptr _adr, uptr _bytes) {
			adr = _adr;
			bytes = _bytes;
		}
		void unset() {
			bytes = 0;
		}

		uptr adr;
		uptr bytes;  ///< unused if bytes == 0.

		bichain_node<range> _chain_node;
		bichain_node<range>& chain_hook() { return _chain_node; }
	};

	typedef bichain<range, &range::chain_hook> range_chain;

	struct adr_slot
	{
		range_chain free_ranges;
		range_chain alloc_ranges;
	};
	adr_slot slots[SLOT_COUNT];

	range range_buf[BUF_COUNT];

private:
	static bool is_masked(slot_index i, slot_mask m) {
		return ((1 << i) & m) != 0;
	}
	range* new_range(uptr adr, uptr bytes);
	void free_range(range* e);
	bool   _reserve(uptr adr, uptr bytes, bool forget, adr_slot* slot);
	range* _alloc(uptr bytes, uptr align, bool forget, adr_slot* slot);
	bool   _free(void* p, adr_slot* slot);

public:
	void _debug_dump(adr_slot& slot) {
	 for(range*r=slot.free_ranges.head();r;r=slot.free_ranges.next(r))
	  log()("free:adr:").u(r->adr,16)(";bytes=").u(r->bytes,16)();
	 for(range*r=slot.alloc_ranges.head();r;r=slot.alloc_ranges.next(r))
	  log()("alloc:adr:").u(r->adr,16)(";bytes=").u(r->bytes,16)();
	}
	void debug_dump() {
		for (unsigned i = 0; i < SLOT_COUNT; ++i) {
			log()("slot[").u(i)("]:")();
			_debug_dump(slots[i]);
		}
	}
};

/// @brief  Add memory to free_chain.
//
/// @retval true  Succeeds.
/// @retval false No enough working memory.
template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::add_free(slot_index slot, uptr adr, uptr bytes)
{
	if (bytes == 0)
		return true;

	range* ent = new_range(adr, bytes);
	if (ent == 0)
		return false;

	slots[slot].free_ranges.insert_head(ent);

	return true;
}

/// @brief 指定したメモリ範囲をメモリ確保の対象外にする。
/// @param[in] slotm  slot mask.
/// @param[in] adr    取り除くメモリの先頭アドレス。
/// @param[in] bytes  取り除くメモリのバイト数。
/// @param[in] forget true ならばメモリ確保の対象外にしたことを忘れる。
/// @retval true  Succeeds.
/// @retval false No enough working memory.
///               The cheap_alloc object might be broken.
//
/// - メモリ確保の対象外にしたい領域がある場合は、メモリを確保する前に
///   この関数で設定しなければならない。
/// - forget の扱いは alloc() の forget と同じで、割り当て可能なメモリが
///   あったこと自体を忘れてしまう。
/// - forget = false ならば、対象外にしたメモリ範囲を enum_xxx() で
///   拾うことができる。
template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::reserve(
    slot_mask slotm,
    uptr adr, uptr bytes,
    bool forget)
{
	for (slot_index i = 0; i < SLOT_COUNT; ++i) {
		if (!is_masked(i, slotm))
			continue;

		if (_reserve(adr, bytes, forget, &slots[i]) == false)
			return false;
	}

	return true;
}

/// @brief  Allocate memory.
/// @param[in] slotm  Slot mask.
/// @param[in] bytes  required memory size.
/// @param[in] align  Memory aliment. Must to be 2^n.
///     EX: If align == 256, then return address is "0x....00".
/// @param[in] forget Forget this memory.
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns 0.
//
/// - forget = true とすると、このメモリを割り当てたこと自体を忘れてしまう。
/// - free() で開放するときは forget = false としなければならない。
/// - free() で開放しない場合でも、割り当てたことを覚えておいて、
///   後で enum_alloc() で知りたいときは forget = false としなければならない。
/// - forget = true とすると、add_free() で avoid = true を指定したメモリを
///   優先的に割り当てる。
///   forget = true ならばメモリ管理の終了とともにメモリが開放されるという
///   考え方なので、avoid のメモリを避ける必要はないということ。
template<int BUF_COUNT>
void* cheap_alloc<BUF_COUNT>::alloc(
    slot_mask slotm,
    uptr bytes, uptr align,
    bool forget)
{
	range* ent;
	for (slot_index i = 0; i < SLOT_COUNT; ++i) {
		if (!is_masked(i, slotm))
			continue;

		ent = _alloc(bytes, align, forget, &slots[i]);
		if (ent)
			break;
	}

	return reinterpret_cast<void*>(ent->adr);
}

/// @brief  Free memory.
/// @param[in] p  Ptr to memory to free.
template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::free(slot_mask slotm, void* p)
{
	bool r;
	for (slot_index i = 0; i < SLOT_COUNT; ++i) {
		if (!is_masked(i, slotm))
			continue;

		r = _free(p, &slots[i]);
		if (r)
			break;
	}

	return r;
}

template<int BUF_COUNT>
void cheap_alloc<BUF_COUNT>::enum_free(slot_mask slotm, enum_desc* x) const
{
	x->slotm = slotm;
	x->index = 0;
	x->range_node = 0;
}

template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::enum_free_next(
    enum_desc* x, uptr* adr, uptr* bytes) const
{
	for (slot_index i = x->index; i < SLOT_COUNT; ++i) {
		if (!is_masked(i, x->slotm))
			continue;

		x->index = i;
		if (x->range_node)
			x->range_node =
			    slots[i].free_ranges.next(x->range_node);
		else
			x->range_node =
			    slots[i].free_ranges.head();

		if (x->range_node)
			break;
	}

	if (x->range_node) {
		*adr = x->range_node->adr;
		*bytes = x->range_node->bytes;
		return true;
	} else {
		return false;
	}
}

template<int BUF_COUNT>
void cheap_alloc<BUF_COUNT>::enum_alloc(slot_mask slotm, enum_desc* x) const
{
	x->slotm = slotm;
	x->index = 0;
	x->range_node = 0;
}

template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::enum_alloc_next(
    enum_desc* x, uptr* adr, uptr* bytes) const
{
	for (slot_index i = x->index; i < SLOT_COUNT; ++i) {
		if (!is_masked(i, x->slotm))
			continue;

		x->index = i;
		if (x->range_node)
			x->range_node =
			    slots[i].alloc_ranges.next(x->range_node);
		else
			x->range_node =
			    slots[i].alloc_ranges.head();

		if (x->range_node)
			break;
	}

	if (x->range_node) {
		*adr = x->range_node->adr;
		*bytes = x->range_node->bytes;
		return true;
	} else {
		return false;
	}
}

/// @brief  Search unused entry from entry_buf[].
/// @return  If unused entry found, this func returns ptr to unused entry.
/// @return  If unused entry not found, this func returns 0.
template<int BUF_COUNT>
typename cheap_alloc<BUF_COUNT>::range*
cheap_alloc<BUF_COUNT>::new_range(uptr adr, uptr bytes)
{
	range* const buf = range_buf;

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
void cheap_alloc<BUF_COUNT>::free_range(range* e)
{
	if (e)
		//e->bytes = 0;  // means free.
		e->unset();
}

/// @brief メモリ範囲を空きメモリ(slot->free_ranges)から取り除く。
/// @param[in] adr       取り除くメモリの先頭アドレス。
/// @param[in] bytes     取り除くメモリの長さ。
/// @param[in] forget    Forget this address range.
/// @param[in,out] slot  Address slot. Must not be null.
/// @retval true  Succeeds.
/// @retval false No enough working memory.
///               The cheap_alloc object might be broken.
//
/// - forget = false ならば slot->free_ranges から取り除いたメモリは
///   slot->alloc_ranges に格納する。
/// - forget = true ならば slot->free_ranges からメモリを取り除くだけ。
///   slot->alloc_ranges は操作しない。
template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::_reserve(
    uptr adr, uptr bytes,
    bool forget,
    adr_slot* slot)
{
	uptr r_head = adr;
	uptr r_tail = adr + bytes - 1;

	for (range* ent = slot->free_ranges.head(); ent; )
	{
		uptr e_head = ent->adr;
		uptr e_tail = e_head + ent->bytes - 1;

		if (e_head < r_head && r_tail < e_tail) {
			if (forget == false) {
				range* alloc = new_range(adr, bytes);
				if (!alloc)
					return false;
				slot->alloc_ranges.insert_head(alloc);
			}

			range* free2 = new_range(r_tail + 1, e_tail - r_tail);
			if (!free2)
				return false;
			slot->free_ranges.insert_head(free2);

			ent->bytes = r_head - e_head;
			break;
		}

		if (r_head <= e_head && e_head <= r_tail) {
			if (forget == false) {
				range* alloc = new_range(
				    e_head, min(r_tail, e_tail) - e_head + 1);
				if (!alloc)
					return false;
				slot->alloc_ranges.insert_head(alloc);
			}

			e_head = r_tail + 1;
		}

		if (r_head <= e_tail && e_tail <= r_tail) {
			if (forget == false) {
				const uptr h = max(r_head, e_head);
				range* alloc = new_range(h, e_tail - h + 1);
				if (!alloc)
					return false;
				slot->alloc_ranges.insert_head(alloc);
			}

			e_tail = r_head - 1;
		}

		if (e_head > e_tail) {
			range* next_ent = slot->free_ranges.next(ent);
			slot->free_ranges.remove(ent);
			free_range(ent);
			ent = next_ent;
		} else {
			ent->set(e_head, e_tail - e_head + 1);
			ent = slot->free_ranges.next(ent);
		}
	}

	return true;
}

/// @brief  空きメモリを割り当てる。
/// @param[in] bytes     メモリサイズ。
/// @param[in] align     アライメント。
/// @param[in] forget    Forget this address range.
/// @param[in,out] slot  Address slot. Must not be null.
//
/// - forget = false ならば slot->free_ranges から割り当てたメモリは
///   slot->alloc_ranges に格納する。
/// - forget = true ならば slot->free_ranges からメモリを割り当てるだけ。
///   slot->alloc_ranges は無視する。
template<int BUF_COUNT>
typename cheap_alloc<BUF_COUNT>::range*
cheap_alloc<BUF_COUNT>::_alloc(
    uptr bytes, uptr align,
    bool forget,
    adr_slot* slot)
{
	uptr align_gap;
	range* ent;
	for (ent = slot->free_ranges.head();
	     ent;
	     ent = slot->free_ranges.next(ent))
	{
		align_gap = up_align(ent->adr, align) - ent->adr;
		if ((ent->bytes - align_gap) >= bytes)
			break;
	}
	if (!ent)
		return 0;

	range* r;
	if (ent->bytes == bytes) {
		r = ent;
		slot->free_ranges.remove(ent);
		if (forget == false)
			slot->alloc_ranges.insert_head(ent);
	} else {
		if (align_gap != 0) {
			range* newent = new_range(ent->adr, align_gap);
			if (!newent)
				return 0;

			slot->free_ranges.insert_head(newent);

			ent->adr += align_gap;
			ent->bytes -= align_gap;
		}
		r = new_range(ent->adr, bytes);
		if (!r)
			return 0;

		if (forget == false)
			slot->alloc_ranges.insert_head(r);

		ent->adr += bytes;
		ent->bytes -= bytes;
	}

	return r;
}

template<int BUF_COUNT>
bool cheap_alloc<BUF_COUNT>::_free(void* p, adr_slot* slot)
{
	// 割り当て済みリストから p を探す。

	uptr adr = reinterpret_cast<uptr>(p);
	range* ent;
	for (ent = slot->alloc_ranges.head();
	     ent;
	     ent = slot->alloc_ranges.next(ent))
	{
		if (ent->adr == adr) {
			slot->alloc_ranges.remove(ent);
			break;
		}
	}

	if (!ent)
		return false;

	// 空きリストから p の直後のアドレスを探す。
	// p と p の直後のアドレスを結合して ent とする。

	uptr tail = adr + ent->bytes;
	for (range* ent2 = slot->free_ranges.head();
	     ent2;
	     ent2 = slot->free_ranges.next(ent2))
	{
		if (ent2->adr == tail) {
			ent->bytes += ent2->bytes;
			slot->free_ranges.remove(ent2);
			ent2->unset();
			break;
		}
	}

	// 空きリストから p の前のアドレスを探す。
	// p と p の前のアドレスを結合する。

	adr = ent->adr;
	for (range* ent2 = slot->free_ranges.head();
	     ent2;
	     ent2 = slot->free_ranges.next(ent2))
	{
		if ((ent2->adr + ent2->bytes) == adr) {
			ent2->bytes += ent->bytes;
			ent->unset();
			ent = 0;
			break;
		}
	}

	if (ent)
		slot->free_ranges.insert_head(ent);

	return true;
}


/// @brief cheap_alloc で add_free() を使うときに、アドレス範囲とスロットを
///        関連付ける。
/// @tparam CHEAP_ALLOC cheap_alloc<>の型をそのまま指定する。
//
/// 使い方
/// @code
/// typedef cheap_alloc<64> allocator;
/// typedef cheap_alloc_separator<allocator> separator;
/// allocator a;
/// separator s(&a);
/// s.set_slot_range(0,    0,  999);
/// s.set_slot_range(1, 1000, 1999);
/// s.set_slot_range(2, 2000, 2999);
/// s.add_free_range(500, 2499);
/// @endcode
/// 結果
/// - a.SLOT[0] -> 500-999
/// - a.SLOT[1] -> 1000-1999
/// - a.SLOT[2] -> 2000-2499
template <class CHEAP_ALLOC>
class cheap_alloc_separator
{
public:
	typedef typename CHEAP_ALLOC::slot_index slot_index;

	enum { SLOT_COUNT = CHEAP_ALLOC::SLOT_COUNT };

	explicit cheap_alloc_separator(CHEAP_ALLOC* ea) : cheap_alloc(ea) {}

	void set_slot_range(slot_index i, uptr head, uptr tail);

	bool add_free_range(uptr head, uptr tail);

	bool add_free(uptr adr, uptr bytes) {
		return add_free_range(adr, adr + bytes - 1);
	}

private:
	CHEAP_ALLOC* cheap_alloc;

	struct slot_range {
		bool enable;
		uptr head;
		uptr tail;
	};
	slot_range slots[SLOT_COUNT];

	bool _add_free_range(slot_index i, uptr free_head, uptr free_tail);
};

/// @brief  スロットのアドレス範囲を指定する。
//
/// アドレス範囲が adr/bytes 形式のときは、
/// set_slot_range(x, adr, adr + bytes - 1)
/// のように指定する。
template<class CHEAP_ALLOC>
void cheap_alloc_separator<CHEAP_ALLOC>::set_slot_range(
    slot_index i,
    uptr head,
    uptr tail)
{
	slots[i].enable = true;
	slots[i].head = head;
	slots[i].tail = tail;
}

/// @brief  cheap_alloc::add_free() を呼び出す。
//
/// set_slot_range() で指定したアドレス範囲に振り分ける。
template<class CHEAP_ALLOC>
bool cheap_alloc_separator<CHEAP_ALLOC>::add_free_range(uptr head, uptr tail)
{
	for (slot_index i = 0; i < SLOT_COUNT; ++i) {
		if (slots[i].enable) {
			if (!_add_free_range(i, head, tail))
				return false;
		}
	}

	return true;
}

template<class CHEAP_ALLOC>
bool cheap_alloc_separator<CHEAP_ALLOC>::_add_free_range(
    slot_index i, uptr free_head, uptr free_tail)
{
	const slot_range& slot = slots[i];

	if (!(slot.head <= free_tail && slot.tail >= free_head))
		return true;

	const uptr add_head = max(free_head, slot.head);
	const uptr add_tail = min(free_tail, slot.tail);

	if (add_head > add_tail)
		return true;

	return cheap_alloc->add_free(i, add_head, add_tail - add_head + 1);
}


#endif  // include guard
