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
	/// 64 * 2 blocks
	u64 free_mem_bitmap[2];
public:
	bichain_link<physical_4kmemblk_cluster> chain_item;
};

typedef
    bichain<physical_4kmemblk_cluster, &physical_4kmemblk_cluster::chain_item>
    pmem_cluster_chain;

pmem_cluster_chain free_chain;
pmem_cluster_chain used_chain;

u64 get_physical_memory_end()
{
	setup_memmgr_dumpdata* memmap;
	u32 memmap_num;
	setup_get_free_memmap(&memmap, &memmap_num);

	// ここで memmap の最後の方から物理メモリの終端アドレスを計算する。
}

} // namespace

cause::stype phymemmgr_init()
{
	
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
