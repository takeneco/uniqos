/// @file   core/process.hh
/// @brief  process class declaration.
//
// (C) 2013-2015 KATO Takeshi
//

#ifndef CORE_PROCESS_HH_
#define CORE_PROCESS_HH_

#include <core/basic.hh>
#include <core/io_node.hh>
#include <core/thread.hh>


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

	chain_node<process>& get_process_ctl_node() {
		return process_ctl_node;
	}

	process_id get_process_id() const { return id; }

	cause::t setup(thread* entry_thread, int iod_nr);
	cause::t set_io_desc_nr(int nr);

	cause::pair<io_desc*> get_io_desc(int i);
	cause::t clear_io_desc(int iod);
	cause::t set_io_desc(int iod, io_node* target, io_node::offset off);
	cause::pair<int> append_io_desc(io_node* io, io_node::offset off);

private:
	fchain<thread, &thread::process_chainnode> child_thread_chain;
	chain_node<process> process_ctl_node;
	process_id id;

	int io_desc_nr;
	io_desc** io_desc_array;
};

process* get_current_process();


#endif  // include guard

