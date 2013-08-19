/// @file   log_target.cc
/// @brief  log destination.
//
// (C) 2009-2013 KATO Takeshi
//

#include <log_target.hh>


//TODO
io_node::operations log_target_io_node_ops;

cause::type log_target::setup()
{
	log_target_io_node_ops.init();

	log_target_io_node_ops.write = call_on_io_node_write<log_target>;

	return cause::OK;
}

log_target::log_target() :
	target_node(0)
{
}

void log_target::install(io_node* target, offset off)
{
	ops = &log_target_io_node_ops;

	target_node = target;
	target_off = off;
}

cause::t log_target::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	if (!this || !target_node)
		return cause::FAIL;

#ifdef KERNEL
	spin_lock_section sec(write_lock);
#endif

	offset pre_target_off = target_off;

	cause::t r = target_node->write(&target_off, iov_cnt, iov);

	*off += target_off - pre_target_off;

	return r;
}

