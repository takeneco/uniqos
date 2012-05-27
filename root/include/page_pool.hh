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

	uptr calc_workarea_size(uptr pmem_end_);

	bool init(uptr pmem_end_, void* buf);
	bool load_setupdump();
	bool load_free_range(u64 adr, u64 bytes);
	void build();

	cause::stype alloc(arch::page::TYPE pt, uptr* padr);
	cause::stype free(arch::page::TYPE pt, uptr padr);

private:
	mem_cell_base<u64> page_base[arch::page::LEVEL_COUNT];
	uptr pmem_end;
};


#endif  // include guard

