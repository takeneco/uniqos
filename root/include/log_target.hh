/// @file  log_target.hh
//
// (C) 2008-2012 KATO Takeshi
//

#ifndef INCLUDE_LOG_TARGET_HH_
#define INCLUDE_LOG_TARGET_HH_

#include <io_node.hh>
#include <spinlock.hh>


class log_target : public io_node
{
public:
	static cause::type setup();

	void install(io_node* target, offset off = 0);

	cause::type on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	io_node*   target_node;
	offset     target_off;
	spin_lock  write_lock;
};


#endif  // include guard

