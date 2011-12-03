/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

enum { LOG_MODES = 2 };
log_target* log_tgt[LOG_MODES];

} // namespace

void log_set(u8 i, log_target* target)
{
	if (i < LOG_MODES)
		log_tgt[i] = target;
}

log_target& log(u8 i)
{
	return i < LOG_MODES ?
	    *log_tgt[i] : *static_cast<log_target*>(0);
}

