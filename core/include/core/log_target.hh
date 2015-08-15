/// @file  log_target.hh
//
// (C) 2008-2014 KATO Takeshi
//

#ifndef CORE_LOG_TARGET_HH_
#define CORE_LOG_TARGET_HH_

#include <core/io_node.hh>
#include <core/spinlock.hh>


class log_target : public io_node
{
public:
	static cause::t setup();

	log_target();

	void install(io_node* target, offset off = 0);

	cause::pair<uptr> on_Write(offset off, const void* data, uptr bytes);
	cause::t on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	io_node*   target_node;
	offset     target_off;
	spin_lock  write_lock;
};


#endif  // CORE_LOG_TARGET_HH_

