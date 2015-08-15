/// @file   log_target.cc
/// @brief  log destination.
//
// (C) 2009-2014 KATO Takeshi
//

#include <core/log_target.hh>


//TODO
io_node::interfaces log_target_io_node_ifs;

cause::t log_target::setup()
{
	log_target_io_node_ifs.init();

	log_target_io_node_ifs.Write = call_on_Write<log_target>;
	log_target_io_node_ifs.write = call_on_io_node_write<log_target>;

	return cause::OK;
}

log_target::log_target() :
	target_node(0)
{
}

void log_target::install(io_node* target, offset off)
{
	ifs = &log_target_io_node_ifs;

	target_node = target;
	target_off = off;
}

cause::pair<uptr> log_target::on_Write(
    offset /*off*/, const void* data, uptr bytes)
{
	if (!this || !target_node)
		return zero_pair(cause::FAIL);

#ifdef KERNEL
	spin_lock_section sec(write_lock);
#endif

	auto r = target_node->write(target_off, data, bytes);

	target_off += r.get_data();

	return r;
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

	cause::t r = target_node->writev(&target_off, iov_cnt, iov);

	*off += target_off - pre_target_off;

	return r;
}

