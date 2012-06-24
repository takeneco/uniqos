/// @file  page_pool.cc
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_PAGE_POOL_HH_
#define INCLUDE_PAGE_POOL_HH_

#include <arch.hh>
#include <memcell.hh>


/// @brief Page control
class page_pool
{

public:
	page_pool();

	void set_range(uptr low_adr, uptr high_adr);
	uptr calc_workarea_bytes();

	bool init(uptr mem_bytes, void* buf);
	bool load_free_range(uptr adr, uptr bytes);
	void build();

	cause::stype alloc(arch::page::TYPE pt, uptr* padr);
	cause::stype dealloc(arch::page::TYPE pt, uptr padr);

private:
	mem_cell_base<uptr> page_base[arch::page::LEVEL_COUNT];
	uptr adr_offset;
	uptr pool_bytes;
};


#endif  // include guard

