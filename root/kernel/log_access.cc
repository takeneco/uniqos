/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

log_target* log_tgt;
unsigned int log_mask;

} // namespace

void log_init(log_target* target)
{
	log_tgt = target;
	log_mask = 0x00000001;
}

log_target& log(u8 i)
{
	return log_mask & (1 << i) ?
	    *log_tgt : *static_cast<log_target*>(0);
}

void log_set(u8 i, bool mask)
{
	if (mask)
		log_mask |= 1 << i;
	else
		log_mask &= ~(1 << i);
}

