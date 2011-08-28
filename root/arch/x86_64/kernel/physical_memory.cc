/// @file  physical_memory.cc
/// @brief Physical memory management.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch.hh"
#include "basic_types.hh"
#include "bitmap.hh"
#include "chain.hh"
#include "global_variables.hh"
#include "native_ops.hh"
#include "memory_allocate.hh"
#include "memcell.hh"
#include "placement_new.hh"
#include "setupdata.hh"
#include "boot_access.hh"

#include "output.hh"


namespace {

using global_variable::gv;

/// @brief 物理メモリの末端アドレスを返す。
//
/// セットアップ後の空きメモリのうち、
/// 最大のアドレスを物理メモリの末端アドレスとして計算する。
u64 get_pmem_end()
{
	setup_memory_dumpdata* freemap;
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
/// @return 適当に見つけた setup_memory_dumpdata のポインタを返す。
/// @return 無い場合は nullptr を返す。
setup_memory_dumpdata* search_free_pmem(u32 size)
{
	setup_memory_dumpdata* freemap;
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

/// @brief セットアップ後の空きメモリ情報を直接操作し、メモリを割り当てる。
void* mem_alloc(u32 size)
{
	setup_memory_dumpdata* pmem_data = search_free_pmem(size);
	if (pmem_data == 0)
		return 0;

	const uptr base_padr =
	    up_align<uptr>(pmem_data->head, arch::BASIC_TYPE_ALIGN);

	// 空きメモリからメモリ管理用領域を外す。
	const uptr align_diff = base_padr - pmem_data->head;
	pmem_data->head += size + align_diff;
	pmem_data->bytes -= size + align_diff;

	return arch::pmem::direct_map(base_padr);
}

}  // namepsace

/////////////////////////////////////////////////////////////////////
/// @brief 物理メモリの空き状態を管理する。
class physical_memory
{
	mem_cell_base<u64> page_base[5];

public:
	uptr calc_workarea_size(uptr pmem_end_);

	physical_memory();

	bool init(uptr pmem_end_, void* buf);
	bool load_setupdump();
	void build();

	cause::stype reserve_l1page(uptr* padr);
	cause::stype reserve_l2page(uptr* padr);

	cause::stype free_l1page(uptr padr);
	cause::stype free_l2page(uptr padr);

	cause::stype page_alloc(arch::PAGE_TYPE pt, uptr* padr);
	cause::stype page_free(arch::PAGE_TYPE pt, uptr padr);
};

/// @brief 物理メモリの管理に必要なワークエリアのサイズを返す。
//
/// @param[in] _pmem_end 物理メモリの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr physical_memory::calc_workarea_size(uptr _pmem_end)
{
	return page_base[4].calc_buf_size(_pmem_end);
}

physical_memory::physical_memory()
{
	page_base[0].set_params(12, 0);
	page_base[1].set_params(18, &page_base[0]);
	page_base[2].set_params(21, &page_base[1]);
	page_base[3].set_params(27, &page_base[2]);
	page_base[4].set_params(30, &page_base[3]);
}

/// @param[in] _pmem_end 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool physical_memory::init(uptr _pmem_end, void* buf)
{
	page_base[4].set_buf(buf, _pmem_end);

	return true;
}

bool physical_memory::load_setupdump()
{
	setup_memory_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memdump(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; ++i) {
		page_base[4].free_range(
		    freemap[i].head,
		    freemap[i].head + freemap[i].bytes - 1);
	}

	return true;
}

void physical_memory::build()
{
	page_base[4].build_free_chain();
}

cause::stype physical_memory::reserve_l1page(uptr* padr)
{
	return page_base[0].reserve_1page(padr);
}

cause::stype physical_memory::reserve_l2page(uptr* padr)
{
	return page_base[2].reserve_1page(padr);
}

cause::stype physical_memory::free_l1page(uptr padr)
{
	return page_base[0].free_1page(padr);
}

cause::stype physical_memory::free_l2page(uptr padr)
{
	return page_base[2].free_1page(padr);
}

cause::stype physical_memory::page_alloc(arch::PAGE_TYPE pt, uptr* padr)
{
	return page_base[pt].reserve_1page(padr);
}

cause::stype physical_memory::page_free(arch::PAGE_TYPE pt, uptr padr)
{
	return page_base[pt].free_1page(padr);
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

	void* buf = mem_alloc(sizeof (physical_memory));
	physical_memory* pmem_ctrl =
	    new (buf) physical_memory;
	if (pmem_ctrl == 0)
		return cause::NO_MEMORY;

	buf = mem_alloc(pmem_ctrl->calc_workarea_size(pmem_end));
	if (buf == 0)
		return cause::NO_MEMORY;

	// 続くメモリをバッファにする。
	pmem_ctrl->init(pmem_end, buf);

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

