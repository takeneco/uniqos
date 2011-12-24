/// @file  log.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_LOG_HH_
#define INCLUDE_LOG_HH_

#include "log_target.hh"


class log : public log_target
{
public:
	log(u32 type=0);
	~log();
};

void log_init(int type, file* target);


#endif  // include guard
