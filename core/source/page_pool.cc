/// @file  page_pool.cc
/// @brief Physical page pool.
//
// (C) 2012 KATO Takeshi
//

#include <core/page_pool.hh>

#include <core/log.hh>


/** page_pool initialize how to
 * 1. Call constructor.
 * 2. Set page range by add_range() or copy_range_from().
 * 3. Calc buf size by calc_workbuf_bytes().
 * 4. Allocate buf and call init().
 * 5. Mark free memory range by load_free_range().
 * 6. Create free page chain by build().
 * 7. Use it.
 */

page_pool::page_pool(u32 _proximity_domain) :
	page_range_cnt(0),
	proximity_domain(_proximity_domain)
{
	page_base[0].set_params(arch::page::bits_of_level(0), 0);

	for (int i = 1; i <= arch::page::HIGHEST; ++i) {
		page_base[i].set_params(
		    arch::page::bits_of_level(i),
		    &page_base[i - 1]);
	}
}

/// @brief  管理対象物理メモリの範囲を追加する。
cause::t page_pool::add_range(const adr_range& add)
{
	if (page_range_cnt >= num_of_array(page_ranges)) {
		log()("!!!page_pool: too many page_ranges entry.")();
		return cause::FAIL;
	}

	const uptr align_bits = arch::page::bits_of_level(arch::page::HIGHEST);

	if (page_range_cnt == 0) {
		adr_offset = down_align(add.low_adr(), UPTR(1) << align_bits);
		pool_bytes = add.high_adr() - adr_offset + 1;
	} else {
		const uptr l_adr1 = adr_offset;
		const uptr h_adr1 = adr_offset + pool_bytes - 1;
		const uptr l_adr2 = add.low_adr();
		const uptr h_adr2 = add.high_adr();

		const uptr ladr = min(l_adr1, l_adr2);
		const uptr hadr = max(h_adr1, h_adr2);

		adr_offset = down_align(ladr, UPTR(1) << align_bits);
		pool_bytes = hadr - adr_offset + 1;
	}

	page_ranges[page_range_cnt++].set(add);

	return cause::OK;
}

/// @brief  Copy page range from other instance.
void page_pool::copy_range_from(const page_pool& src)
{
	adr_offset = src.adr_offset;
	pool_bytes = src.pool_bytes;

	page_range_cnt = src.page_range_cnt;

	for (uint i = 0; i < page_range_cnt; ++i)
		page_ranges[i].set(src.page_ranges[i]);
}

/// @brief 物理メモリの管理に必要なデータエリアのサイズを返す。
/// @return 必要なデータエリアのサイズをバイト数で返す。
uptr page_pool::calc_workbuf_bytes()
{
	return page_base[arch::page::HIGHEST].calc_buf_size(pool_bytes);
}

/// @param[in] mem_bytes 管理対象メモリの合計サイズ。
/// @param[in] buf  calc_workarea_bytes() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool page_pool::init(uptr buf_bytes, void* buf)
{
	page_base[arch::page::HIGHEST].set_buf(buf, pool_bytes);

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

cause::t page_pool::alloc(page_level level, uptr* padr)
{
	uptr _padr;
	const cause::t r = page_base[level].reserve_1page(&_padr);

	*padr = _padr + adr_offset;
	return r;
}

cause::t page_pool::dealloc(page_level level, uptr padr)
{
	bool hit = false;
	for (uint i = 0; i < page_range_cnt; ++i) {
		if (page_ranges[i].test(padr)) {
			hit = true;
			break;
		}
	}
	if (!hit)
		return cause::OUTOFRANGE;

	padr -= adr_offset;

	return page_base[level].free_1page(padr);
}

void page_pool::dump(output_buffer& ob, uint level)
{
	if (level >= 1)
		ob("--- BEGIN page_pool dump ---")();

	if (level >= 1) {
		ob("adr_offset : ").x(adr_offset)();
		ob("pool_bytes : ").x(pool_bytes)();
	}

	for (uptr i = 0; i < num_of_array(page_base); ++i)
		page_base[i].dump(pool_bytes, ob, level);

	if (level >= 1)
		ob("--- END page_pool dump ---")();
}

