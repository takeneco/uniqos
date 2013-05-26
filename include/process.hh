/// @file   include/process.hh
/// @brief  process class declaration.
//
// (C) 2013 KATO Takeshi
//

#ifndef INCLUDE_PROCESS_HH_
#define INCLUDE_PROCESS_HH_

#include <pagetable.hh>


class process
{
	class page_allocator;

	static void* phys_to_virt(uptr adr) {
		return arch::map_phys_adr(adr, arch::page::PHYS_L1_SIZE);
	}

	typedef arch::page_table<page_allocator, phys_to_virt> pagetable;

public:
	process();
	~process();

	pagetable& ref_ptbl() { return ptbl; }

	cause::t init();

private:
	 pagetable ptbl;
};

class process::page_allocator
{
public:
	cause::t alloc(u64* padr);
	cause::t free(u64 padr);
};


#endif  // include guard
