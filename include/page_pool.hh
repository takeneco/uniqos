/// @file  page_pool.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_PAGE_POOL_HH_
#define CORE_INCLUDE_PAGE_POOL_HH_

#include <arch.hh>
#include <memcell.hh>


/// @brief Page pool
class page_pool
{
public:
	page_pool();

	cause::type add_range(const adr_range& add);
	void set_range(uptr low_adr, uptr high_adr);
	uptr calc_workarea_bytes();

	bool init(uptr mem_bytes, void* buf);
	bool load_free_range(uptr adr, uptr bytes);
	void build();

	cause::type alloc(arch::page::TYPE pt, uptr* padr);
	cause::type dealloc(arch::page::TYPE pt, uptr padr);

	void dump(output_buffer& ob, uint level);

private:
	mem_cell_base<uptr> page_base[arch::page::LEVEL_COUNT];
	uptr adr_offset;
	uptr pool_bytes;

	uint      page_range_cnt;  ///< page_ranges[] のエントリ数
	adr_range page_ranges[4];  ///< page_pool が含むページのアドレス範囲
};


#endif  // CORE_INCLUDE_PAGE_POOL_HH_

