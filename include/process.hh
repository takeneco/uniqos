/// @file   core/include/core/process.hh
/// @brief  process class declaration.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_PROCESS_HH_
#define CORE_INCLUDE_CORE_PROCESS_HH_

#include <basic.hh>
#include <chain.hh>
#include <pagetable.hh>


typedef u32 pid;

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

	bichain_node<process>& get_process_ctl_node() {
		return process_ctl_node;
	}

	pid get_pid() const { return id; }

	pagetable& ref_ptbl() { return ptbl; }

	cause::t init();

private:
	pagetable ptbl;
	bichain_node<process> process_ctl_node;
	pid id;
};

class process::page_table_acquire
{
public:
	static cause::pair<uptr> acquire(pagetable* x);
	static cause::t          release(pagetable* x, u64 padr);
};


#endif  // include guard
