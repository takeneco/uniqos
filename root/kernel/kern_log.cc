/// @file  log.cc
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"


namespace {

file* log_tgt;

}

void log_init(file* target)
{
	log_tgt = target;
}

log::log(u32 /*type*/)
:    log_target(log_tgt)
{
}

log::~log()
{
}

