/// @file   core/process.hh
/// @brief  process class declaration.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef CORE_PROCESS_HH_
#define CORE_PROCESS_HH_

#include <core/basic.hh>
#include <core/io_node.hh>
#include <core/thread.hh>
#include <arch/pagetable.hh>


typedef u32 process_id;

class io_desc
{
public:
	io_node* io;
	io_node::offset off;
};

class process
{
public:
	process();
	~process();

	bichain_node<process>& get_process_ctl_node() {
		return process_ctl_node;
	}

	process_id get_process_id() const { return id; }
	cause::pair<io_desc*> get_io_desc(int i);

	cause::t setup(thread* entry_thread, int iod_nr);
	cause::t set_io_desc_nr(int nr);

	cause::t set_io_desc(int iod, io_node* target);

private:
	bibochain<thread, &thread::process_chainnode> child_thread_chain;
	bichain_node<process> process_ctl_node;
	process_id id;

	int io_desc_nr;
	io_desc** io_desc_array;
};

process* get_current_process();


#endif  // include guard

