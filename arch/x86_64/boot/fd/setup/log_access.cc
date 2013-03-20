/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

kernel_log* log_target;
unsigned int log_mask;

} // namespace

void log_init(kernel_log* target)
{
	log_target = target;
	log_mask = 0x00000001;
}

kernel_log& log(u8 i)
{
	return log_mask & (1 << i) ?
	    *log_target : *static_cast<kernel_log*>(0);
}

void log_set(u8 i, bool mask)
{
	if (mask)
		log_mask |= 1 << i;
	else
		log_mask &= ~(1 << i);
}

