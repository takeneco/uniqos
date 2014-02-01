/// @file   core/include/core/process.hh
/// @brief  process class declaration.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_PROCESS_HH_
#define CORE_INCLUDE_CORE_PROCESS_HH_

#include <core/basic.hh>
#include <chain.hh>
#include <pagetable.hh>


typedef u32 pid;

class process
{
public:
	process();
	~process();

	bichain_node<process>& get_process_ctl_node() {
		return process_ctl_node;
	}

	pid get_pid() const { return id; }

	cause::t init();

private:
	bichain_node<process> process_ctl_node;
	pid id;
};


#endif  // include guard

