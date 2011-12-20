/// @file   log_access.cc
/// @brief  global log interface.
//
// (C) 2011 KATO Takeshi
//

#include "misc.hh"


namespace {

enum { LOG_MODES = 2 };
file* log_tgt[LOG_MODES];

} // namespace

void log_set(uint i, file* target)
{
	if (i < LOG_MODES)
		log_tgt[i] = target;
}

log::log(int i)
:    log_target(log_tgt[i])
{
}

log::~log()
{
}
