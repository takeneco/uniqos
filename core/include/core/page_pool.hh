/// @file  core/page_pool.hh
//
// (C) 2012-2015 KATO Takeshi
//

#ifndef CORE_PAGE_POOL_HH_
#define CORE_PAGE_POOL_HH_

#include <core/pagetbl.hh>
#include <core/memcell.hh>


/// @brief Page pool
class page_pool
{
public:
	page_pool(u32 proximity_domain);

	cause::t add_range(const adr_range& add);
	void copy_range_from(const page_pool& src);
	uptr calc_workbuf_bytes();

	bool init(uptr mem_bytes, void* buf);
	bool load_free_range(uptr adr, uptr bytes);
	void build();

	u32 get_proximity_domain() const { return proximity_domain; }

	cause::t alloc(arch::page::TYPE pt, uptr* padr);
	cause::t dealloc(arch::page::TYPE pt, uptr padr);

	void dump(output_buffer& ob, uint level);

private:
	mem_cell_base<uptr> page_base[arch::page::LEVEL_COUNT];
	uptr adr_offset;
	uptr pool_bytes;

	uint      page_range_cnt;  ///< page_ranges[] のエントリ数
	adr_range page_ranges[4];  ///< page_pool が含むページのアドレス範囲

	u32 proximity_domain;
};


#endif  // include guard

