/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

enum { LOG_MODES = 2 };
kernel_log* log_target[LOG_MODES];

} // namespace

void log_set(u8 i, kernel_log* target)
{
	if (i < LOG_MODES)
		log_target[i] = target;
}

kernel_log& log(u8 i)
{
	return i < LOG_MODES ?
	    *log_target[i] : *static_cast<kernel_log*>(0);
}

