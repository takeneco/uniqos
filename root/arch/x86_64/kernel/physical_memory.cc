/// @author KATO Takeshi
/// @brief  Physical memory manager.
//
// (C) 2010 KATO Takeshi

#include <algorithm>

#include "arch.hh"
#include "btypes.hh"
#include "bitmap.hh"
#include "chain.hh"
#include "global_variables.hh"
#include "native.hh"
#include "pnew.hh"
#include "setupdata.hh"
#include "setup/memdump.hh"

#include "output.hh"


namespace 
{

using global_variable::gv;

/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリページの空き状態を管理する。
class physical_page_table
{
	struct table_item
	{
		bitmap<uptr> table;
		chain_link<table_item> chain_link_;
	};

	typedef chain<table_item, &table_item::chain_link_> table_chain;

	table_chain free_item_chain;
	table_item* table_base;

	const u64 page_size;
	physical_page_table* const next_level;

public:
	enum {
		BITMAP_BITS = bitmap<uptr>::BITS;
	};

	physical_page_table(u64 page_size_, physical_page_table* next_level_)
	:	page_size(page_size_),
		next_level(next_level_)
	{}

	static uptr calc_buf_size(uptr pmem_end, uptr page_size);

	void init(void* buf);
	void init_free_all(void* buf, uptr buf_size);

	void reserve_range(uptr from, uptr to);
};

uptr physical_page_table::calc_buf_size(uptr pmem_end, uptr page_size)
{
	// 物理メモリのページ数
	const uptr pages = pmem_end / page_size;

	// すべてのページを管理するために必要な bitmap_list 数
	const uptr bitmaps = (pages + (BITMAP_BITS - 1)) / BITMAP_BITS;

	return sizeof (table_item) * bitmaps;
}

/// @brief バッファを設定する。
//
/// バッファの初期化はしない。
void physical_page_table::init(void* buf)
{
	table_base = reinterpret_cast<table_item*>(buf);
}

/// @brief 物理メモリ全体を空き状態として初期化する。
void physical_page_table::init_free_all(void* buf, uptr buf_size)
{
	table_base = reinterpret_cast<table_item*>(buf);

	const uptr n = buf_size / sizeof (table_item);

	dechain<table_item, &table_item::chain_link_> dech;

	for (uptr i = 0; i < n; ++i) {
		table_base[i].table.set_true_all();
		dech.insert_tail(&table_base[i]);
	}

	free_item_chain.init_head(dech.get_head());
}

void physical_page_table::reserve_range(uptr from, uptr to)
{
	const uptr fr_page = from / page_size;
	const uptr fr_table = fr_page / BITMAP_BITS;
	const uptr fr_page_in_table = fr_page - (fr_table * BITMAP_BITS);

	const uptr to_page = to / page_size;
	const uptr to_table = to_page / BITMAP_BITS;
	const uptr to_page_in_table = to_page - (t_table * BITMAP_BITS);

	for (uptr table = fr_table; table <= to_table; ++table) {
		const uptr page_min =
		    table == fr_table ? fr_page_in_table : 0;
		const uptr page_max =
		    table == to_table ? to_page_in_table : BITMAP_BITS - 1;

		for (uptr page = page_min; page <= page_max; ++page) {
			table_base[table].table.set_false(page);
		}
	}

	if (next_level != 0) {
		if (from % page_size != 0) {
			const uptr tmp = up_align(from, page_size);
			next_level->reserve_range(from, std::min(tmp, to));
		}
		if (fr_page != to_page && to % page_size != 0) {
			const uptr tmp = down_align(to, page_size);
			next_level->reserve_range(tmp, to);
		}
	}
}

/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリの空き状態を２段のページで管理する。
class physical_memory
{
	physical_page_table page_l1_table;
	physical_page_table page_l2_table;
	uptr pmem_end;

	void reserve_bits(uptr from, uptr to);
	void reserve_range(uptr from, uptr to);

public:
	static uptr calc_workarea_size(uptr pmem_end_);

	physical_memory();

	bool init(uptr pmem_end_, void* buf);
	bool load_setupdump();
};

void physical_memory::reserve_bits(uptr from, uptr to)
{
	
}

/// from から to までを割当済みの状態にする。
void physical_memory::reserve_range(uptr from, uptr to)
{
	const uptr l2size_x_bits = PAGE_L2_SIZE * page_l2_table.BITMAP_BITS;
	if (from % l2size_x_bits != 0) {
		const uptr tmp = std::min(up_align(from, l1size_x_bits), to);
		reserve_bits(from, tmp);
		from = tmp;
	}

	const uptr down_align_to = down_align(to, l2size_x_bits);
	for (; from < down_align_to; from += l2size_x_bits) {
	}

	if (to != down_align_to) {
		reserve_bits(from, to);
	}
}

/// @brief 物理メモリの管理に必要なワークエリアのサイズを返す。
//
/// @param[in] pmem_end_ 物理メモリの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr physical_memory::calc_workarea_size(uptr pmem_end_)
{
	const uptr l1buf = physical_page_table::calc_buf_size(
	    pmem_end_, PAGE_L1_SIZE);
	const uptr l2buf = physical_page_table::calc_buf_size(
	    pmem_end_, PAGE_L2_SIZE);

	return l1buf + l2buf;
}

physical_memory::physical_memory()
:	page_l1_table(PAGE_L1_SIZE, &page_l2_table),
	page_l2_table(PAGE_L2_SIZE, 0)
{
}

/// @param[in] pmem_end_ 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool physical_memory::init(uptr pmem_end_, void* buf)
{
	pmem_end = pmem_end_;

	const uptr l2buf_size =
	    physical_page_table::calc_buf_size(pmem_end_, PAGE_L2_SIZE);
	page_l2_table.init_free_all(buf, l2buf_size);

	page_l1_table.init(reinterpret_cast<char*>(buf) + l2buf_size);

	return true;
}

bool physical_memory::load_setupdump()
{
	setup_memmgr_dumpdata* usedmap;
	u32 usedmap_num;
	setup_get_used_memmap(&usedmap, &usedmap_num);

	for (u32 i = 0; i < usedmap_num; i++) {
		reserve_range(usedmap[i]->head,
		    usedmap[i]->head + usedmap[i]->bytes);
	}
}

/// @brief 物理メモリの末端アドレスを返す。
//
/// セットアップ後の空きメモリのうち、
/// 最大のアドレスを物理メモリの末端アドレスとして計算する。
u64 get_pmem_end()
{
	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memmap(&freemap, &freemap_num);

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
	setup_get_free_memmap(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; i++) {
		const uptr d = up_align(freemap[i].head) - freemap[i].head;
		if ((freemap[i].bytes - d) >= size)
			return &freemap[i];
	}

	return 0;
}

}  // namepsace

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
	    sizeof physical_memory;

	setup_memmgr_dumpdata* pmem_data = search_free_pmem(work_size);
	if (pmem_data == 0)
		return cause::NO_MEMORY;

	const uptr base_padr = up_align(pmem_data->head, 8);

	// 空きメモリからメモリ管理用領域を外す。
	const uptr align_diff = base_padr - pmem_data->head;
	pmem_data->bytes -= work_size + align_diff;
	pmem_data->head += work_size + align_diff;

	// メモリ管理用領域の先頭を physical_memory に割り当てる。
	physical_memory* pmem_ctrl =
	    new (reinterpret_cast<u8*>(PHYSICAL_MEMMAP_BASEADR + base_adr))
	    physical_memory;

	// 続くメモリをバッファにする。
	pmem_ctrl->init(pmem_end, pmem_ctrl + 1);


	// 物理メモリマップのアドレスを渡す。
	_init_table(table_base_addr + 0xffff800000000000, bitmap_num);

	gv.pmem_ctrl = pmem_ctrl;

	return cause::OK;
}

}  // namespace pmem

}  // namespace arch



namespace {

/// メモリブロックの空き状態を管理する。
/// 4KiBメモリブロック単位で管理し、
/// 1bitでメモリブロックの空き状態を記憶する。
class physical_4kmemblk_bitmap
{
	static physical_4kmemblk_bitmap* table_base_addr;

	/// 64 * 1 blocks
	/// 空きメモリを表すビットは１に、
	/// 使用メモリを表すビットは０にする。
	u64 free_mem_bitmap[1];

public:
	chain_link<physical_4kmemblk_bitmap> _chain_link;

	enum {
		BITS = 8 * sizeof free_mem_bitmap,
	};

private:
	void free_bits(u64 head, u64 tail);

	u64 search_freepage() const {
		s64 r = bitscan_forward_64(free_mem_bitmap[0]);
		// assert(r >= 0)
		return static_cast<int>(r);
	}
	void set_free(int i) {
		free_mem_bitmap[0] |= 1 << i;
	}
	void unset_free(int i) {
		free_mem_bitmap[0] &= ~(1 << i);
	}

public:
	static void set_table_base_addr(u64 base) {
		table_base_addr =
		    reinterpret_cast<physical_4kmemblk_bitmap*>(base);
	}
	static physical_4kmemblk_bitmap* get_bitmap_by_addr(u64 addr) {
		return &table_base_addr[addr / arch::PAGE_SIZE / BITS];
	}

	physical_4kmemblk_bitmap(setup_memmgr_dumpdata* freemap, u32 num);

	bool is_empty() const {
		return free_mem_bitmap[0] == 0;
	}

	u64 get_baseadr() const {
		return (this - table_base_addr) * arch::PAGE_SIZE * BITS;
	}
	u64 get_page_baseadr(int i) const {
		return get_baseadr() + i * arch::PAGE_SIZE;
	}

	bool alloc_1page(u64* padr);

	u64 test() { return free_mem_bitmap[0]; }
};

typedef
    dechain<physical_4kmemblk_bitmap, &physical_4kmemblk_bitmap::_chain_link>
    pmem_bitmap_chain;

/// 空きメモリを含む physical_4kmemblk_bitmap の片方向リスト。
pmem_bitmap_chain free_chain;


physical_4kmemblk_bitmap* physical_4kmemblk_bitmap::table_base_addr;

/// メモリ範囲内のページを空き状態にする。
/// @param[in] head メモリ範囲の先頭アドレス
/// @param[in] tail メモリ範囲の終端アドレス+1
void physical_4kmemblk_bitmap::free_bits(
    u64 freehead, u64 freetail)
{
	u64 pagehead = get_baseadr();
	u64 pagetail = pagehead + arch::PAGE_SIZE;

	for (u32 i = 0; i < BITS; ++i) {
		if (freehead <= pagehead && pagetail <= freetail)
			free_mem_bitmap[0] |= 1 << i;
	}
}

physical_4kmemblk_bitmap::physical_4kmemblk_bitmap(
    setup_memmgr_dumpdata* freemap,
    u32 num)
	: _chain_link()
{
	free_mem_bitmap[0] = 0;

	const u64 blkhead = get_baseadr();
	const u64 blktail = blkhead + arch::PAGE_SIZE * BITS;

	for (u32 i = 0; i < num; ++i) {
		const u64 freehead = freemap[i].head;
		const u64 freetail = freehead + freemap[i].bytes;
		if (freehead <= blkhead && blktail <= freetail) {
			free_mem_bitmap[0] = 0xffffffffffffffff;
			break;
		}
		else if ((freehead <= blkhead && blkhead <  freetail) ||
		         (freehead <  blktail && blktail <= freetail) ||
			 (blkhead <= freehead && freetail <= blktail))
		{
			free_bits(freehead, freetail);
		}
	}
}

bool physical_4kmemblk_bitmap::alloc_1page(u64* padr)
{
	const int i = search_freepage();

	unset_free(i);

	*padr = get_page_baseadr(i);

	return true;
}



void _init_table(u64 table_base_addr, u64 table_num)
{
	new(&free_chain) pmem_bitmap_chain;

	physical_4kmemblk_bitmap::set_table_base_addr(table_base_addr);

	physical_4kmemblk_bitmap* bitmap =
	    reinterpret_cast<physical_4kmemblk_bitmap*>(table_base_addr);

	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memmap(&freemap, &freemap_num);

	for (u64 i = 0; i < table_num; i++) {
		new(&bitmap[i]) physical_4kmemblk_bitmap(freemap, freemap_num);
		if (!bitmap[i].is_empty())
			free_chain.insert_head(&bitmap[i]);

		kern_get_out()->put_str("bm[")->put_udec((u32)i)->
		put_str("] = ")->put_u64hex(bitmap[i].test())->put_endl();
	}

	for (u32 i = 0; i < freemap_num; ++i) {
		kern_get_out()->put_u64hex(freemap[i].head)->put_c(':')
		->put_u64hex(freemap[i].head+freemap[i].bytes)->put_endl();
	}
}

bool _alloc_1page(u64* padr)
{
	physical_4kmemblk_bitmap* bitmap = free_chain.get_head();
	if (!bitmap)
		return false;

	if (bitmap->is_empty()) {
		// bitmap は先頭から取り出したので、先頭を取り除く。
		free_chain.remove_head();
	}

	return bitmap->alloc_1page(padr);
}

} // namespace

namespace arch {

namespace pmem {

/// 物理メモリページを１ページ確保する。
/// @param[out] padr  Ptr to physical page base address returned.
/// @retval cause::FAIL  No free memory.
/// @retval cause::OK  Succeeds. *padr is physical page base address.
cause::stype alloc_page(u64* padr)
{
	return _alloc_1page(padr) ? cause::OK : cause::FAIL;
}

}  // namespace pmem

}  // namespace arch
