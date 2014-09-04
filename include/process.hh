/// @file   core/process.hh
/// @brief  process class declaration.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef CORE_PROCESS_HH_
#define CORE_PROCESS_HH_

#include <core/basic.hh>
#include <core/io_node.hh>
#include <pagetable.hh>


class thread;

typedef u32 process_id;

class process
{
public:
	process();
	~process();

	bichain_node<process>& get_process_ctl_node() {
		return process_ctl_node;
	}

	process_id get_process_id() const { return id; }

	cause::t init(thread* entry_thread, int iod_nr);
	cause::t set_io_desc(int iod, io_node* target);
	cause::pair<thread*> create_thread();

private:
	bichain_node<process> process_ctl_node;
	process_id id;

	int io_desc_nr;
	io_node** io_desc;
};

process* get_current_process();


#endif  // include guard

