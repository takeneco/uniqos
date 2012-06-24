/// @file  page_pool.cc
/// @brief Physical page pool.
//
// (C) 2012 KATO Takeshi
//

#include <page_pool.hh>


page_pool::page_pool()
{
	page_base[0].set_params(arch::page::bits_of_level(0), 0);

	for (int i = 1; i <= arch::page::HIGHEST; ++i) {
		page_base[i].set_params(
		    arch::page::bits_of_level(i),
		    &page_base[i - 1]);
	}
}

/// @brief  管理対象物理メモリの範囲を指定する。
void page_pool::set_range(uptr low_adr, uptr high_adr)
{
	const uptr align = arch::page::bits_of_level(arch::page::HIGHEST);

	adr_offset = down_align(low_adr, align);
	pool_bytes = high_adr - adr_offset + 1;
}

/// @brief 物理メモリの管理に必要なデータエリアのサイズを返す。
/// @return 必要なデータエリアのサイズをバイト数で返す。
uptr page_pool::calc_workarea_bytes()
{
	return page_base[arch::page::HIGHEST].calc_buf_size(pool_bytes);
}

/// @param[in] mem_bytes 管理対象メモリの合計サイズ。
/// @param[in] buf  calc_workarea_bytes() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool page_pool::init(uptr buf_bytes, void* buf)
{
	page_base[arch::page::HIGHEST].set_buf(buf, buf_bytes);

	return true;
}

bool page_pool::load_free_range(uptr adr, uptr bytes)
{
	adr -= adr_offset;

	page_base[arch::page::HIGHEST].free_range(adr, adr + bytes - 1);

	return true;
}

void page_pool::build()
{
	page_base[arch::page::HIGHEST].build_free_chain();
}

cause::stype page_pool::alloc(arch::page::TYPE page_type, uptr* padr)
{
	uptr _padr;
	const cause::type r = page_base[page_type].reserve_1page(&_padr);

	*padr = _padr + adr_offset;
	return r;
}

cause::stype page_pool::dealloc(arch::page::TYPE page_type, uptr padr)
{
	padr -= adr_offset;

	return page_base[page_type].free_1page(padr);
}

