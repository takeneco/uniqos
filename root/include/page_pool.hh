/// @file  page_pool.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_PAGE_POOL_HH_
#define INCLUDE_PAGE_POOL_HH_

#include <arch.hh>
#include <memcell.hh>


/// @brief Page pool
class page_pool
{
public:
	page_pool();

	void set_range(uptr low_adr, uptr high_adr);
	uptr calc_workarea_bytes();

	bool init(uptr mem_bytes, void* buf);
	bool load_free_range(uptr adr, uptr bytes);
	void build();

	cause::type alloc(arch::page::TYPE pt, uptr* padr);
	cause::type dealloc(arch::page::TYPE pt, uptr padr);

	void dump(output_buffer& ob) {
		ob("--- BEGIN page_pool dump ---")();
		ob("adr_offset : ").x(adr_offset)();
		ob("pool_bytes : ").x(pool_bytes)();
		for (uptr i = 0; i < num_of_array(page_base); ++i)
			page_base[i].dump(pool_bytes, ob);
		ob("--- END page_pool dump ---")();
	}

private:
	mem_cell_base<uptr> page_base[arch::page::LEVEL_COUNT];
	uptr adr_offset;
	uptr pool_bytes;
};


#endif  // include guard

