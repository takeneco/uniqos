/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "misc.hh"


namespace {

enum { LOG_MODES = 1 };
file* log_tgt[LOG_MODES];

} // namespace

void log_set(uint i, file* target)
{
	if (i < LOG_MODES)
		log_tgt[i] = target;
}

log::log()
:    log_target(log_tgt[0])
{
}

log::~log()
{
}
