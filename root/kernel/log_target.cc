/// @file   log_target.cc
/// @brief  log destination.
//
// (C) 2009-2012 KATO Takeshi
//

#include <log_target.hh>


io_node::operations log_target_io_node_ops;

cause::type log_target::setup()
{
	log_target_io_node_ops.init();

	log_target_io_node_ops.write = call_on_io_node_write<log_target>;

	return cause::OK;
}

void log_target::install(io_node* target, offset off)
{
	ops = &log_target_io_node_ops;

	target_node = target;
	target_off = off;
}

cause::type log_target::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	offset pre_target_off = target_off;

	cause::type r = target_node->write(&target_off, iov_cnt, iov);

	*off += target_off - pre_target_off;

	return r;
}

