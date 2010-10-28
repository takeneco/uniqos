/// @author KATO Takeshi
/// @brief  Physical memory manager.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "btypes.hh"
#include "bitmap.hh"
#include "chain.hh"
#include "global_variables.hh"
#include "kerninit.hh"
#include "native.hh"
#include "placement_new.hh"
#include "setupdata.hh"
#include "setup/memdump.hh"

#include "output.hh"


namespace {

using global_variable::gv;

const uptr ADR_INVALID = U64CAST(0xffffffffffffffff);

/// @brief 物理メモリの末端アドレスを返す。
//
/// セットアップ後の空きメモリのうち、
/// 最大のアドレスを物理メモリの末端アドレスとして計算する。
u64 get_pmem_end()
{
	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memdump(&freemap, &freemap_num);

	u64 total_end = 0L;
	for (u32 i = 0; i < freemap_num; i++) {
		const u64 end = freemap[i].head + freemap[i].bytes;
		if (end > total_end)
			total_end = end;
	}

	return total_end;
}

/// @brief セットアップ後の空きメモリ情報から、
///        連続した物理メモリの空き領域を探す。
//
/// @param[in] size 必要な最小のメモリサイズ。
/// @return 適当に見つけた setup_memmgr_dumpdata のポインタを返す。
/// @return 無い場合は nullptr を返す。
setup_memmgr_dumpdata* search_free_pmem(u32 size)
{
	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memdump(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; i++) {
		const uptr d =
		    up_align<u8>(freemap[i].head, 8) - freemap[i].head;

		if ((freemap[i].bytes - d) >= size)
			return &freemap[i];
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリページの空き状態を管理する。
class physical_page_table
{
	/// bitmap の１ビットで１ページの空き状態を管理する。
	struct table_cell
	{
		/// 空きページのビットは true
		/// 予約ページのビットは false
		bitmap<uptr> table;
		chain_link<table_cell> chain_link_;
	};

	typedef chain<table_cell, &table_cell::chain_link_> cell_chain;

	// １ページ * BITMAP_BITS のサイズを 1cell と呼ぶことにする。

	/// 空きメモリチェイン
	cell_chain free_cell_chain;
	table_cell* table_base;

	const uptr page_size;
	const uptr cell_size;
	physical_page_table* const up_level;
	physical_page_table* const down_level;

private:
	uptr get_page_address(const table_cell* cell, uptr offset) const;
	unsigned int get_cell_index(uptr adr) const;
	table_cell& get_cell(uptr adr);
	bool import_uplevel_page();

public:
	enum {
		BITMAP_BITS = bitmap<uptr>::BITS,
	};

	physical_page_table(
	    u64 page_size_,
	    physical_page_table* up_level_,
	    physical_page_table* down_level_)
	:	page_size(page_size_),
		cell_size(page_size * BITMAP_BITS),
		up_level(up_level_),
		down_level(down_level_)
	{}

	static uptr calc_buf_size(uptr pmem_end, uptr page_size);

	void init_reserve_all(void* buf, uptr buf_size);
	void init_free_all(void* buf, uptr buf_size);

	void reserve_range(uptr from, uptr to);
	uptr free_range(uptr from, uptr to);

	void build_free_chain(uptr pmem_end);

	uptr reserve_1page();
	void free_1page(uptr padr);

	void dump() {
		for (int i = 0; i < 128; i++) {
			if (!table_base[i].table.is_all_false()) {
			kern_get_out()->put_u32hex(i)->put_c(':')->
			put_u64hex(table_base[i].table.get_raw())->put_c('\n');
			}
		}
	}
};

inline
uptr physical_page_table::get_page_address(
    const physical_page_table::table_cell* cell,
    uptr offset)
const
{
	return ((cell - table_base) * BITMAP_BITS + offset) * page_size;
}

inline
unsigned int physical_page_table::get_cell_index(uptr adr) const
{
	return adr / cell_size;
}

inline
physical_page_table::table_cell& physical_page_table::get_cell(uptr adr)
{
	return table_base[get_cell_index(adr)];
}

/// 上のレベルから1ページだけ崩して取り込む。
/// @retval true  Succeeds.
/// @retval false No memory in uplevel.
bool physical_page_table::import_uplevel_page()
{
	if (up_level == 0)
		return false;

	uptr up_padr = up_level->reserve_1page();
	if (up_padr == ADR_INVALID)
		return false;

	const uptr up_page_size = up_level->page_size;
	for (uptr offset = 0; offset < up_page_size; offset += cell_size) {
		table_cell& cell = get_cell(up_padr + offset);
		free_cell_chain.insert_head(&cell);
	}

	return true;
}

uptr physical_page_table::calc_buf_size(uptr pmem_end, uptr page_size)
{
	// 物理メモリのページ数
	const uptr pages = (pmem_end + (page_size - 1)) / page_size;

	// すべてのページを管理するために必要な bitmap_list 数
	const uptr bitmaps = (pages + (BITMAP_BITS - 1)) / BITMAP_BITS;

	return sizeof (table_cell) * bitmaps;
}

/// @brief 物理メモリを予約済みとして初期化する。
/// Without for top levels.
void physical_page_table::init_reserve_all(void* buf, uptr buf_size)
{
	table_base = reinterpret_cast<table_cell*>(buf);

	const uptr n = buf_size / sizeof (table_cell);

	// up_level から渡されたページを free の状態にするため、
	// true で初期化したい。

	for (uptr i = 0; i < n; ++i)
		table_base[i].table.set_false_all();
}

/// @brief 物理メモリ全体を空き状態として初期化する。
//
// *** UNUSED ***
//
void physical_page_table::init_free_all(void* buf, uptr buf_size)
{
	table_base = reinterpret_cast<table_cell*>(buf);

	const uptr n = buf_size / sizeof (table_cell);

	dechain<table_cell, &table_cell::chain_link_> dech;

	for (uptr i = 0; i < n; ++i) {
		table_base[i].table.set_true_all();
		dech.insert_tail(&table_base[i]);
	}

	free_cell_chain.init_head(dech.get_head());
}

/// @brief from から to までを割当済みの状態にする。
//
/// 最上位のレベルでなければ使えない。
/// 下のレベルのテーブルにも反映する。
/// 空きメモリリストは更新しない。
//
// *** UNUSED ***
//
void physical_page_table::reserve_range(uptr from, uptr to)
{
	const uptr fr_page = from / page_size;
	const uptr fr_table = fr_page / BITMAP_BITS;
	const uptr fr_page_in_table = fr_page % BITMAP_BITS;

	const uptr to_page = to / page_size;
	const uptr to_table = to_page / BITMAP_BITS;
	const uptr to_page_in_table = to_page % BITMAP_BITS;

	for (uptr table = fr_table; table <= to_table; ++table) {
		const uptr page_min =
		    table == fr_table ? fr_page_in_table : 0;
		const uptr page_max =
		    table == to_table ? to_page_in_table : BITMAP_BITS - 1;

		for (uptr page = page_min; page <= page_max; ++page) {
			table_base[table].table.set_false(page);
		}
	}

	if (down_level != 0) {
		if (from % page_size != 0) {
			const uptr tmp = down_align(from, page_size);
			down_level->free_range(tmp, from);
		}
		if (to % page_size != 0) {
			const uptr tmp = up_align(to, page_size);
			down_level->free_range(to, tmp);
		}
	}
}

/// @brief from から to-1 までを空き状態にする。
/// @return 空き状態になったメモリのバイト数を返す。
/// @return この関数を呼び出す前に空き領域になっていたメモリの分も含む。
//
/// page_size より小さな空きメモリは、下のレベルのテーブルに反映する。
/// 空きメモリチェインは更新しない。
uptr physical_page_table::free_range(uptr from, uptr to)
{
	const uptr fr_page = up_align(from, page_size) / page_size;
	const uptr fr_table = fr_page / BITMAP_BITS;
	const uptr fr_page_in_table = fr_page % BITMAP_BITS;

	const uptr to_page = (down_align(to, page_size) - 1) / page_size;
	const uptr to_table = to_page / BITMAP_BITS;
	const uptr to_page_in_table = to_page % BITMAP_BITS;

	uptr total = 0;
	for (uptr table = fr_table; table <= to_table; ++table) {
		if (table != fr_table && table != to_table) {
			table_base[table].table.set_true_all();
			total += cell_size;
			continue;
		}

		const uptr page_min =
		    table == fr_table ? fr_page_in_table : 0;
		const uptr page_max =
		    table == to_table ? to_page_in_table : BITMAP_BITS - 1;

		for (uptr page = page_min; page <= page_max; ++page) {
			table_base[table].table.set_true(page);
			total += page_size;
		}
	}

	if (down_level != 0) {
		if (from % page_size != 0) {
			const uptr tmp = up_align(from, page_size);
			total += down_level->free_range(from, min(tmp, to));
		}
		if (fr_page <= to_page && to % page_size != 0) {
			const uptr tmp = down_align(to, page_size);
			total += down_level->free_range(tmp, to);
		}
	}

	return total;
}

/// @brief 空きメモリチェインをつなぐ。
void physical_page_table::build_free_chain(uptr pmem_end)
{
	dechain<table_cell, &table_cell::chain_link_> dech;

	const uptr n = (pmem_end + (page_size - 1)) / page_size;
	for (uptr i = 0; i < n; ++i) {
		if (!table_base[i].table.is_all_false())
			dech.insert_tail(&table_base[i]);
	}

	free_cell_chain.init_head(dech.get_head());
}

/// @brief １ページだけ予約する。
/// @return 予約した物理メモリのアドレス返す。
/// @return 空きメモリがないときは ADR_INVALID を返す。
uptr physical_page_table::reserve_1page()
{
	table_cell* cell;

	for (;;) {
		cell = free_cell_chain.get_head();

		if (cell == 0) {
			if (import_uplevel_page())
				continue;
			else
				return ADR_INVALID;
		}

		if (!cell->table.is_all_false())
			break;

		// まとまった cell を上位レベルに返却するときに、
		// 一緒に返却された cell は、空きページを含まないまま
		// free_cell_chain に残ってしまう。
		// その場合はここで cell を削除する。
		free_cell_chain.remove_head();
	}

	const int offset = cell->table.search_true();
	cell->table.set_false(offset);
	if (cell->table.is_all_false())
		free_cell_chain.remove_head();

	return get_page_address(cell, offset);
}

/// @brief １ページだけ解放する。
/// @param[in] padr 解放する物理メモリのアドレス。
///                 ページサイズより小さなビットは無視してしまう。
void physical_page_table::free_1page(uptr padr)
{
	const unsigned int cell_index = get_cell_index(padr);
	table_cell& cell = table_base[cell_index];
	if (cell.table.is_all_false())
		free_cell_chain.insert_head(&cell);

	const int offset = padr / page_size % BITMAP_BITS;
	cell.table.set_true(offset);

	if (cell.table.is_all_true() && up_level) {
		// padr を含む上位レベルのページがすべて空き状態となった
		// 場合は、上位レベルへ返却する。

		// 上位レベルの１ページは n セルに相当する。
		const uptr n = up_level->page_size / this->cell_size;
		const unsigned int up_cell_base = cell_index / n * n;

		unsigned int i;
		for (i = 0; i < n; ++i) {
			if (!table_base[up_cell_base + i].table.is_all_true())
				break;
		}

		if (i == n) {
			for (i = 0; i < n; ++i) {
				table_base[up_cell_base + i].
				    table.set_false_all();
			}

			up_level->free_1page(padr);
		}
	}
}

}  // namepsace

/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリの空き状態を２段のページで管理する。
class physical_memory
{
	physical_page_table page_l1_table;
	physical_page_table page_l2_table;
	uptr pmem_end;
	uptr free_bytes;

public:
	static uptr calc_workarea_size(uptr pmem_end_);

	physical_memory();

	bool init(uptr pmem_end_, void* buf);
	bool load_setupdump();
	void build();

	cause::stype reserve_l1page(uptr* padr);
	cause::stype reserve_l2page(uptr* padr);

	cause::stype free_l1page(uptr padr);
	cause::stype free_l2page(uptr padr);

	void tmp_debug() {
		page_l1_table.dump();
	}
};

/// @brief 物理メモリの管理に必要なワークエリアのサイズを返す。
//
/// @param[in] pmem_end_ 物理メモリの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr physical_memory::calc_workarea_size(uptr pmem_end_)
{
	const uptr l1buf = physical_page_table::calc_buf_size(
	    pmem_end_, arch::PAGE_L1_SIZE);
	const uptr l2buf = physical_page_table::calc_buf_size(
	    pmem_end_, arch::PAGE_L2_SIZE);

	return l1buf + l2buf;
}

physical_memory::physical_memory()
:	page_l1_table(arch::PAGE_L1_SIZE, &page_l2_table, 0),
	page_l2_table(arch::PAGE_L2_SIZE, 0, &page_l1_table),
	free_bytes(0)
{
}

/// @param[in] pmem_end_ 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool physical_memory::init(uptr pmem_end_, void* buf)
{
	pmem_end = pmem_end_;

	const uptr l2buf_size =
	    physical_page_table::calc_buf_size(pmem_end_, arch::PAGE_L2_SIZE);
	page_l2_table.init_reserve_all(buf, l2buf_size);

	const uptr l1buf_size =
	    physical_page_table::calc_buf_size(pmem_end_, arch::PAGE_L1_SIZE);
	page_l1_table.init_reserve_all(
	    reinterpret_cast<char*>(buf) + l2buf_size,
	    l1buf_size);

	return true;
}

bool physical_memory::load_setupdump()
{
	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memdump(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; ++i) {
		free_bytes += page_l2_table.free_range(
		    freemap[i].head,
		    freemap[i].head + freemap[i].bytes);
	}

	return true;
}

void physical_memory::build()
{
	page_l1_table.build_free_chain(pmem_end);
	page_l2_table.build_free_chain(pmem_end);
}

cause::stype physical_memory::reserve_l1page(uptr* padr)
{
	uptr tmp = page_l1_table.reserve_1page();
	if (tmp != ADR_INVALID) {
		*padr = tmp;
		return cause::OK;
	}
	else {
		return cause::NO_MEMORY;
	}
}

cause::stype physical_memory::reserve_l2page(uptr* padr)
{
	uptr tmp = page_l2_table.reserve_1page();
	if (tmp != ADR_INVALID) {
		*padr = tmp;
		return cause::OK;
	}
	else {
		return cause::NO_MEMORY;
	}
}

cause::stype physical_memory::free_l1page(uptr padr)
{
	page_l1_table.free_1page(padr);

	return cause::OK;
}

cause::stype physical_memory::free_l2page(uptr padr)
{
	page_l2_table.free_1page(padr);

	return cause::OK;
}

namespace arch
{

namespace pmem
{

/// @retval cause::NO_MEMORY  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype init()
{
	// 物理メモリの終端アドレス。これを物理メモリサイズとする。
	const uptr pmem_end = get_pmem_end();

	const uptr work_size =
	    physical_memory::calc_workarea_size(pmem_end) +
	    sizeof (physical_memory);

	setup_memmgr_dumpdata* pmem_data = search_free_pmem(work_size);
	if (pmem_data == 0)
		return cause::NO_MEMORY;

	const uptr base_padr = up_align<uptr>(pmem_data->head, 8);

	// 空きメモリからメモリ管理用領域を外す。
	const uptr align_diff = base_padr - pmem_data->head;
	pmem_data->head += work_size + align_diff;
	pmem_data->bytes -= work_size + align_diff;


	// メモリ管理用領域の先頭を physical_memory に割り当てる。
	physical_memory* pmem_ctrl =
	    new (reinterpret_cast<u8*>(PHYSICAL_MEMMAP_BASEADR + base_padr))
	    physical_memory;

	// 続くメモリをバッファにする。
	pmem_ctrl->init(pmem_end, pmem_ctrl + 1);

	pmem_ctrl->load_setupdump();

	pmem_ctrl->build();

	gv.pmem_ctrl = pmem_ctrl;

	return cause::OK;
}

/// レベル１の物理メモリページを１ページ確保する。
/// @param[out] padr  Ptr to physical page base address returned.
/// @retval cause::FAIL  No free memory.
/// @retval cause::OK  Succeeds. *padr is physical page base address.
cause::stype alloc_l1page(uptr* adr)
{
	return gv.pmem_ctrl->reserve_l1page(adr);
}

/// レベル２の物理メモリページを１ページ確保する。
/// @param[out] padr  Ptr to physical page base address returned.
/// @retval cause::FAIL  No free memory.
/// @retval cause::OK  Succeeds. *padr is physical page base address.
cause::stype alloc_l2page(uptr* adr)
{
	return gv.pmem_ctrl->reserve_l2page(adr);
}

/// レベル１の物理メモリページを１ページ解放する。
/// @param[in] padr  Ptr to physical page base address.
/// @retval cause::OK  Succeeds.
cause::stype free_l1page(uptr adr)
{
	return gv.pmem_ctrl->free_l1page(adr);
}

/// レベル２の物理メモリページを１ページ解放する。
/// @param[in] padr  Ptr to physical page base address.
/// @retval cause::OK  Succeeds.
cause::stype free_l2page(uptr adr)
{
	return gv.pmem_ctrl->free_l2page(adr);
}

}  // namespace pmem

}  // namespace arch

void tmp_debug()
{
	gv.pmem_ctrl->tmp_debug();
}
