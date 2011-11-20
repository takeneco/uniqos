/// @file  page_ctl.cc
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_PAGE_CTL_HH_
#define ARCH_X86_64_INCLUDE_PAGE_CTL_HH_

#include "memcell.hh"


class page_ctl
{
	mem_cell_base<u64> page_base[5];

	bool pse;     ///< page-size extensions for 32bit paging.
	bool pae;     ///< physical-address extension.
	bool pge;     ///< global-page support.
	bool pat;     ///< page-attribute table.
	bool pse36;   ///< 36bit page size extension.
	bool pcid;    ///< process-context identifiers.
	bool nx;      ///< execute disable.
	bool page1gb; ///< 1GByte pages.
	bool lm;      ///< IA-32e mode support.

	int padr_width;
	int vadr_width;

	void detect_paging_features();

public:
	page_ctl();

	uptr calc_workarea_size(uptr pmem_end_);

	bool init(uptr pmem_end_, void* buf);
	bool load_setupdump();
	bool load_free_range(u64 adr, u64 bytes);
	void build();

	cause::stype alloc(arch::page::TYPE pt, uptr* padr);
	cause::stype free(arch::page::TYPE pt, uptr padr);
};


#endif  // include guard

