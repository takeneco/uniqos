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
	class page_table_acquire;

	static void* phys_to_virt(uptr adr) {
		return arch::map_phys_adr(adr, arch::page::PHYS_L1_SIZE);
	}

	typedef arch::page_table<page_table_acquire, phys_to_virt> pagetable;

public:
	process();
	~process();

	pagetable& ref_ptbl() { return ptbl; }

	cause::t init();

private:
	 pagetable ptbl;
};

class process::page_table_acquire
{
public:
	static cause::pair<uptr> acquire(pagetable* x);
	static cause::t          release(pagetable* x, u64 padr);
};


#endif  // include guard
