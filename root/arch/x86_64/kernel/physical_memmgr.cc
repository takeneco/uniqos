// @file   arch/x86_64/kernel/physical_memmgr.cc
// @author Kato Takeshi
// @brief  Physical memory manager.
//
// (C) 2010 Kato Takeshi.

#include "btypes.hh"
#include "chain.hh"
#include "native.hh"
#include "output.hh"
#include "pnew.hh"
#include "setupdata.hh"
#include "setup/memdump.hh"
#include "x86_64.hh"


namespace 
{

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
		return &table_base_addr[addr / PAGE_SIZE / BITS];
	}

	physical_4kmemblk_bitmap(setup_memmgr_dumpdata* freemap, u32 num);

	bool is_empty() const {
		return free_mem_bitmap[0] == 0;
	}

	u64 get_baseadr() const {
		return (this - table_base_addr) * PAGE_SIZE * BITS;
	}
	u64 get_page_baseadr(int i) const {
		return get_baseadr() + i * PAGE_SIZE;
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
	u64 pagetail = pagehead + PAGE_SIZE;

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
	const u64 blktail = blkhead + PAGE_SIZE * BITS;

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


/// 物理メモリサイズをバイト数で返す。
/// セットアップ後の空きメモリのうち、
/// 最大のアドレスを物理メモリの終端アドレスとして計算する。
u64 get_physical_memory_size()
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

/// セットアップ後の空きメモリ情報から、
/// 連続した物理メモリの空き領域を探す。
/// @param[in] size 必要な最小のメモリサイズ。
/// @return 適当に見つけた setup_memmgr_dumpdata のポインタを返す。
/// @return 無い場合は nullptr を返す。
setup_memmgr_dumpdata* search_seriesof_free_physical_memory(u64 size)
{
	setup_memmgr_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memmap(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; i++) {
		if (freemap[i].bytes >= size)
			return &freemap[i];
	}

	return 0;
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

/// @retval cause::NO_MEMORY  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype phymemmgr_init()
{
	enum {
		BITS = physical_4kmemblk_bitmap::BITS,
	};

	const u64 phymem_size = get_physical_memory_size();

	// 物理メモリのページ数
	const u64 phymem_pages = phymem_size / 0x1000;
	// すべての物理メモリページを管理するために必要な bitmap 数
	const u64 bitmap_num = (phymem_pages + (BITS - 1)) / BITS;
	// Required memory size for physical memory management.
	const u64 phymemmgr_buf_size =
	    bitmap_num * sizeof (physical_4kmemblk_bitmap);

	setup_memmgr_dumpdata* phymem_data =
	    search_seriesof_free_physical_memory(phymemmgr_buf_size);
	if (phymem_data == 0)
		return cause::NO_MEMORY;

	const u64 table_base_addr = up_align(8, phymem_data->head);
	phymem_data->bytes -= phymemmgr_buf_size +
	    (table_base_addr - phymem_data->head);
	phymem_data->head += phymemmgr_buf_size;

	// 物理メモリマップのアドレスを渡す。
	_init_table(table_base_addr + 0xffff800000000000, bitmap_num);

	return cause::OK;
}

/// 物理メモリページを１ページ確保する。
/// @param[out] padr  Ptr to physical page base address returned.
/// @retval cause::FAIL  No free memory.
/// @retval cause::OK  Succeeds. padr is physical page base address.
cause::stype phymemmgr_alloc_1page(u64* padr)
{
	return _alloc_1page(padr) ? cause::OK : cause::FAIL;
}
