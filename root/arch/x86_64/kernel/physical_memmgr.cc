// @file   arch/x86_64/kernel/physical_memmgr.cc
// @author Kato Takeshi
// @brief  Physical memory manager.
//
// (C) 2010 Kato Takeshi.

#include "btypes.hh"
#include "chain.hh"
#include "setup/memdump.hh"

namespace 
{

/// メモリブロックの空き状態を管理する。
/// 4KiBメモリブロック単位で管理し、
/// 1bitで空き状態を記憶する。
class physical_4kmemblk_cluster
{
	static u64 cluster_base_addr;

	/// 64 * 2 blocks
	u64 free_mem_bitmap[2];
public:
	bichain_link<physical_4kmemblk_cluster> chain_item;

public:
	static void set_base_addr(u64 base) {
		cluster_base_addr = base;
	}
	u64 get_base_addr();
};
u64 physical_4kmemblk_cluster::cluster_base_addr;

typedef
    bichain<physical_4kmemblk_cluster, &physical_4kmemblk_cluster::chain_item>
    pmem_cluster_chain;

pmem_cluster_chain free_chain;
pmem_cluster_chain used_chain;

inline u64 physical_4kmemblk_cluster::get_base_addr()
{
	return reinterpret_cast<u64>(this) - cluster_base_addr;
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

} // namespace

/// @retval cause::NO_MEMORY  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype phymemmgr_init()
{
	const u64 phymem_size = get_physical_memory_size();

	setup_memmgr_dumpdata* phymem_data =
	    search_seriesof_free_physical_memory(phymem_size);
	if (phymem_data == 0)
		return cause::NO_MEMORY;

	// 物理メモリのページ数
	const u64 phymem_pages = phymem_size / 0x1000;
	// すべての物理メモリページを管理するために必要な cluster 数
	const u64 cluster_num = (phymem_pages + 127) / 128;
	// cluster が使用するメモリサイズ
	const u64 phymemmgr_buf_size =
	    cluster_num * sizeof (physical_4kmemblk_cluster);

	const u64 cluster_base_addr = phymem_data->head;
	phymem_data->head += phymemmgr_buf_size;
	phymem_data->bytes -= phymemmgr_buf_size;

	physical_4kmemblk_cluster* cluster =
	    reinterpret_cast<physical_4kmemblk_cluster>
	    (cluster_base_addr + 0xffff800000000000);

	physical_4kmemblk_cluster::set_base_addr(cluster_base_addr);



	return cause::OK;
}

/// 空きメモリをメモリ管理に取り込む。
// TODO
void merge_free_memblock(u64 head, u64 size)
{
	u64 tail = head + size;

	head = up_align(0x1000, head);
	tail = down_align(0x1000, tail);

	while (head < tail) {
		if ((tail - head) >= 0x40000000 && (head & 0x3fffffff)) {
			// 1GiB block
		}
		else if ((tail - head) >= 0x200000 && (head & 0x1fffff)) {
			// 2MiB block
		}
		else {
			// 4KiB block
		}
	}
}
