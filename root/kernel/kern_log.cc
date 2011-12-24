/// @file  log.cc
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

file* log_tgt[2];

}

void log_init(int type, file* target)
{
	log_tgt[type] = target;
}

log::log(u32 type)
:    log_target(log_tgt[type])
{
}

log::~log()
{
}

