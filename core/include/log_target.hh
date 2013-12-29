/// @file  log_target.hh
//
// (C) 2008-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_LOG_TARGET_HH_
#define CORE_INCLUDE_LOG_TARGET_HH_

#include <io_node.hh>
#include <spinlock.hh>


class log_target : public io_node
{
public:
	static cause::type setup();

	log_target();

	void install(io_node* target, offset off = 0);

	cause::type on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	io_node*   target_node;
	offset     target_off;
	spin_lock  write_lock;
};


#endif  // CORE_INCLUDE_LOG_TARGET_HH_

