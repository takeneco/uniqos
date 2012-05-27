/// @file  page_pool.cc
/// @brief Physical page pool.
//
// (C) 2012 KATO Takeshi
//

#include "page_pool.hh"
#include "pagetable.hh"


page_pool::page_pool()
{
	page_base[0].set_params(12, 0);
	page_base[1].set_params(18, &page_base[0]);
	page_base[2].set_params(21, &page_base[1]);
	page_base[3].set_params(27, &page_base[2]);
	page_base[4].set_params(30, &page_base[3]);
}

/// @brief 物理メモリの管理に必要なデータエリアのサイズを返す。
//
/// @param[in] _pmem_end 物理アドレスの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr page_pool::calc_workarea_size(uptr _pmem_end)
{
	return page_base[4].calc_buf_size(_pmem_end);
}

/// @param[in] _pmem_end 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool page_pool::init(uptr _pmem_end, void* buf)
{
	pmem_end = _pmem_end;

	page_base[4].set_buf(buf, _pmem_end);

	detect_paging_features();

	return true;
}

bool page_pool::load_free_range(u64 adr, u64 bytes)
{
	page_base[4].free_range(adr, adr + bytes - 1);

	return true;
}

void page_pool::build()
{
	page_base[4].build_free_chain();
}

cause::stype page_pool::alloc(arch::page::TYPE page_type, uptr* padr)
{
	return page_base[page_type].reserve_1page(padr);
}

cause::stype page_pool::free(arch::page::TYPE page_type, uptr padr)
{
	return page_base[page_type].free_1page(padr);
}

