/// @file  memcell.cc
/// @brief Physical memory management.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "bitmap.hh"
#include "chain.hh"
#include "placement_new.hh"

#include "log.hh"


/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリページの空き状態を管理する。
///
/// 複数のページをまとめて cell とし、メモリ全体を cell の配列として管理する。
/// ページサイズごとにレベルを分けて管理する。
/// cell のサイズは、その上位レベルの page のサイズになる。
///
/// たとえば 4KB, 256KB, 2MB のページサイズで管理するときは、
/// level[0] page_size = 4KB,   cell_size = 256KB
/// level[1] page_size = 256KB, cell_size = 2MB
/// level[2] page_size = 2MB,   cell_size = (auto)
/// の mem_cell_base を作る。
///
/// 初期化方法
/// (1) set_params() でパラメータを指定する。
///     下位レベル(ページサイズが小さいレベル)から順に呼び出す必要がある。
/// (2) 最上位レベルの calc_buf_size() で必要なバッファサイズを計算する。
/// (3) 最上位レベルの set_buf() でバッファを割り当てる。
/// (4) 最上位レベルの free_range() で空きメモリの範囲を指定する。
///     連続した空きメモリは1回の free_range() で指定しないと、境界付近の
///     空きメモリをうまく取り込めない。
/// (5) build_free_chain() で空きメモリチェインをつなぐ。
///
template<class CELLTYPE>
class mem_cell_base
{
public:
	mem_cell_base() :
		free_chain()
	{}

	void set_params(
	    uptr _page_size_bits,
	    mem_cell_base<CELLTYPE>* _down_level);

	uptr calc_buf_size(uptr mem_size) const;

	void set_buf(void* buf, uptr mem_size);

	void reserve_range(uptr from, uptr to);
	void free_range(uptr from, uptr to);

	void build_free_chain();

	cause::stype reserve_1page(uptr* padr);
	cause::stype free_1page(uptr padr);

private:
	/// メモリアドレスを表現するときは uptr を使う。
	/// page と cell は演算中に負数になるので sptr を使う。
	typedef sptr page_t;
	typedef sptr cell_t;

	/// 連続する page をまとめて cell と呼ぶ。
	/// int_bitset の１ビットで 1 page の空き状態を管理する。
	struct cell
	{
		/// 空きページのビットは true
		/// 予約ページのビットは false
		int_bitset<CELLTYPE> table;

		bichain_link<cell> _chain_link;
		bichain_link<cell>& chain_hook() { return _chain_link; }
	};
	enum {
		BITMAP_BITS = int_bitset<CELLTYPE>::BITS,
	};

	/// すべて空き状態の cell.table のビットパターン
	CELLTYPE free_pattern;
	void init_free_pattern() {
		// やりたいのは、
		// free_pattern = (1 << pages_in_cell()) - 1
		// なのだが、
		// IA のシフト命令はレジスタ幅以上のシフトができない。
		free_pattern =
		    (CELLTYPE(-1) >> (BITMAP_BITS - pages_in_cell()));
	}

	typedef bichain<cell, &cell::chain_hook> cell_chain;

	/// 空きページを含む cell
	cell_chain free_chain;

	page_t free_pages;

	/// すべての cell
	cell* cell_table;
	cell_t cell_count;

	/// (page のメモリサイズ) = 1 << page_size_bits
	u16 page_size_bits;

	/// (cell のメモリサイズ) = 1 << cell_size_bits;
	/// = (up_level のページサイズ)
	u16 cell_size_bits;

	mem_cell_base<CELLTYPE>* up_level;
	mem_cell_base<CELLTYPE>* down_level;

private:
	cell_t calc_cell_count(uptr mem_end) const {
		const uptr cell_size = uptr(1) << cell_size_bits;
		return (mem_end + (cell_size - 1)) / cell_size;
	}

	///
	/// transform between address, page, cell
	///

	page_t up_align_page(uptr adr) const {
		return static_cast<page_t>(
		    (adr + (uptr(1) << page_size_bits) - 1) >> page_size_bits);
	}
	page_t down_align_page(uptr adr) const {
		return static_cast<page_t>(adr >> page_size_bits);
	}
	cell_t up_align_cell(uptr adr) const {
		return static_cast<cell_t>(
		    (adr + (uptr(1) << cell_size_bits) - 1) >> cell_size_bits);
	}
	cell_t down_align_cell(uptr adr) const {
		return static_cast<cell_t>(adr >> cell_size_bits);
	}
	/// @param[in] page  global page number. not address.
	/// @return page position in cell.
	page_t page_of_cell(page_t page) const {
		return page &
		    ((uptr(1) << (cell_size_bits - page_size_bits)) - 1);
	}
	/// @return page count in cell.
	page_t pages_in_cell() const {
		return uptr(1) << (cell_size_bits - page_size_bits);
	} 
	/// @param[in] page  global page number. not address.
	/// @return address of page head. (offset is zero)
	uptr page_head_adr(page_t page) const {
		return static_cast<uptr>(page) << page_size_bits;
	}
	/// @param[in] page  global page number. not address.
	/// @return address of page tail. (offset is ff..ff)
	uptr page_tail_adr(page_t page) const {
		return ((static_cast<uptr>(page) + 1) << page_size_bits) - 1;
	}

	uptr get_page_adr(const cell* c, page_t page_of_c) const {
		return static_cast<uptr>(
		    ((c - cell_table) << cell_size_bits) +
		    (page_of_c << page_size_bits));
	}

	cell& get_cell(uptr adr) {
		return cell_table[down_align_cell(adr)];
	}

	bool import_uplevel_page();

public:
	void dump(uptr total_mem)
	{
		log()
		("page_size_bits = ").u(page_size_bits)()
		("cell_size_bits = ").u(cell_size_bits)()
		("up_level   = ")(up_level)()
		("down_level = ")(down_level)()
		("free_pattern = ").u(free_pattern, 16)()
		("free_pages = ").s(free_pages)();

		log()("---- cell internal start ----")();
		const uptr cells =
		    up_div<uptr>(total_mem, U64(1) << cell_size_bits);
		for (uptr i = 0; i < cells; ++i) {
			if (i % 4 == 0)
				log()("[").u(i)("]");
			log()(" ").u(cell_table[i].table.get_raw(), 16);
			if (i % 4 == 3 || i == (cells - 1))
				log()();
		}
		log()("---- cell internal end ----")();
	}
};

/// 下位レベルから順に呼び出さなければならない。
/// 上位レベルがいなければ cell_size_bits は BITMAP_BITS を
/// すべて活用する値にする。
template<class CELLTYPE>
void mem_cell_base<CELLTYPE>::set_params(
    uptr _page_size_bits,
    mem_cell_base<CELLTYPE>* _down_level)
{
	cell_table = 0;
	page_size_bits = _page_size_bits;
	down_level = _down_level;

	// 上のレベルがいないときのデフォルト
	cell_size_bits = page_size_bits + int_size_bits<CELLTYPE>();
	up_level = 0;

	if (down_level) {
		down_level->cell_size_bits = page_size_bits;
		down_level->up_level = this;
	}
}

/// メモリを管理するために必要なバッファサイズを計算する
template<class CELLTYPE>
uptr mem_cell_base<CELLTYPE>::calc_buf_size(uptr mem_size) const
{
	uptr buf_size = sizeof (cell) * calc_cell_count(mem_size);

	if (down_level)
		buf_size += down_level->calc_buf_size(mem_size);

	return buf_size;
}

/// @brief バッファを割り当て、全体を予約済みとして初期化する。
template<class CELLTYPE>
void mem_cell_base<CELLTYPE>::set_buf(void* buf, uptr mem_size)
{
	cell_count = calc_cell_count(mem_size);
	cell_table = new (buf) cell[cell_count];

	init_free_pattern();

	for (cell_t i = 0; i < cell_count; ++i)
		cell_table[i].table.set_false_all();

	if(down_level)
		down_level->set_buf(&cell_table[cell_count], mem_size);
}

/// @brief from_adr から to_adr までを空き状態にする。
/// @return 空き状態になったメモリのバイト数を返す。
/// @return この関数を呼び出す前に空き領域になっていたメモリの分も含む。
//
/// page_size より小さな空きメモリは、下のレベルのテーブルに反映する。
/// 空きメモリチェインは更新しない。
template<class CELLTYPE>
void mem_cell_base<CELLTYPE>::free_range(uptr from_adr, uptr to_adr)
{
	const page_t fr_page = up_align_page(from_adr);
	const cell_t fr_cell = down_align_cell(page_head_adr(fr_page));
	const page_t fr_page_in_cell = page_of_cell(fr_page);

	// if to_adr points end of page (or cell), then includes the page.
	const page_t to_page = down_align_page(to_adr + 1) - 1;
	const cell_t to_cell = to_page >= 0 ?
	    down_align_cell(page_head_adr(to_page)) : -1;
	const page_t to_page_in_cell = page_of_cell(to_page);

	if (fr_page > to_page) {
		if (down_level != 0)
			down_level->free_range(from_adr, to_adr);
		return;
	}

	for (cell_t cell = fr_cell; cell <= to_cell; ++cell) {
		if (cell != fr_cell && cell != to_cell) {
			cell_table[cell].table.set_true_all();
			continue;
		}

		const page_t page_start =
		    cell == fr_cell ? fr_page_in_cell : 0;
		const page_t page_end =
		    cell == to_cell ? to_page_in_cell : pages_in_cell() - 1;

		for (page_t page = page_start; page <= page_end; ++page)
			cell_table[cell].table.set_true(page);
	}

	if (down_level != 0) {
		const uptr fr_page_head_adr = page_head_adr(fr_page);
		if (fr_page_head_adr != from_adr)
		{
			down_level->free_range(
			    from_adr,
			    min(fr_page_head_adr - 1, to_adr));
		}

		const uptr to_page_tail_adr = page_tail_adr(to_page);
		if (to_page_tail_adr != to_adr &&
		   ((fr_page_head_adr - 1) < to_adr))
		{
			down_level->free_range(
			    to_page_tail_adr + 1,
			    to_adr);
		}
	}
}

/// @brief 空きメモリチェインをつなぐ。
template<class CELLTYPE>
void mem_cell_base<CELLTYPE>::build_free_chain()
{
	page_t free_count = 0;

	const int pic = pages_in_cell();

	for (cell_t i = cell_count - 1; i >= 0; --i) {
		if (!cell_table[i].table.is_false_all()) {
			free_chain.insert_head(&cell_table[i]);

			for (int j = 0; j < pic; ++j) {
				if (cell_table[i].table.is_true(j))
					++free_count;
			}
		}
	}

	free_pages = free_count;

	if (down_level)
		down_level->build_free_chain();
}

/// @brief １ページだけ予約する。
/// @param[out] padr 予約した物理ページのアドレスを返す。
/// @retval cause::OK 成功した。
/// @retval cause::NO_MEMORY 空きメモリがない。
template<class CELLTYPE>
cause::stype mem_cell_base<CELLTYPE>::reserve_1page(uptr* padr)
{
	cell* c;

	for (;;) {
		c = free_chain.head();
		if (c)
			break;

		if (!import_uplevel_page())
			return cause::NO_MEMORY;
	}

	const int offset = c->table.search_true();

	c->table.set_false(offset);

	if (c->table.is_false_all())
		free_chain.remove_head();

	*padr = get_page_adr(c, offset);
	return cause::OK;
}

/// @brief １ページだけ解放する。
/// @param[in] padr 解放する物理メモリページのアドレス。
///                 ページサイズより小さなビットは無視してしまう。
/// @retval cause::OK  Succeeded.
template<class CELLTYPE>
cause::stype mem_cell_base<CELLTYPE>::free_1page(uptr padr)
{
	cause::stype r = cause::OK;

	cell& c = get_cell(padr);
	if (c.table.is_false_all())
		free_chain.insert_head(&c);

	const page_t page_of_c = page_of_cell(down_align_page(padr));

	c.table.set_true(page_of_c);

	if (up_level && c.table.get_raw() == free_pattern) {
		// 上位レベルへ返却する。
		c.table.set_false_all();
		r = up_level->free_1page(get_page_adr(&c, 0));
	}

	return r;
}


/// 上のレベルから 1 page だけ崩して 1 cell として取り込む。
/// @retval true  Succeeds.
/// @retval false No memory in uplevel.
template<class CELLTYPE>
bool mem_cell_base<CELLTYPE>::import_uplevel_page()
{
	if (up_level == 0)
		return false;

	uptr up_padr;
	const cause::stype r = up_level->reserve_1page(&up_padr);
	if (r != cause::OK)
		return false;

	cell& c = get_cell(up_padr);

	c.table.set_raw(free_pattern);

	free_chain.insert_head(&c);

	return true;
}

inline void memcell_test()
{
	log()("---- memcell test start")();

	mem_cell_base<u64> mcb[5];
	char buf[8192], *bufp = buf;

	mcb[0].set_params(12, 0);
	mcb[1].set_params(18, &mcb[0]);
	mcb[2].set_params(21, &mcb[1]);
	mcb[3].set_params(27, &mcb[2]);
	mcb[4].set_params(30, &mcb[3]);

	const uptr total_mem = 32 * 1024 * 1024 - 1;
	uptr buf_size;
	buf_size = mcb[4].calc_buf_size(total_mem);
	log()("buf_size = ").u(buf_size)();

	mcb[4].set_buf(bufp, total_mem);

	mcb[4].free_range(16*1024*1024, 32*1024*1024-1);

	mcb[4].build_free_chain();

	for (u32 i = 0; i < 5; ++i) {
		log()("-------- mcb[").u(i)("] = ")(&mcb[i])(" start")();
		mcb[i].dump(total_mem);
		log()("-------- mcb[").u(i)("] end")();
	}

	uptr p[10];
	for (u32 i = 0; i < 10; ++i) {
		mcb[0].reserve_1page(&p[i]);
		log()("reserve from mcb (").u(i)(") = ").u(p[i], 16)();
	}
	for (u32 i = 0; i < 9; ++i) {
		mcb[0].free_1page(p[i]);
	}

	for (u32 i = 0; i < 5; ++i) {
		log()("-------- mcb[").u(i)("] = ")(&mcb[i])(" start")();
		mcb[i].dump(total_mem);
		log()("-------- mcb[").u(i)("] end")();
	}

	log()("---- memcell test end")();
}
