/// @file  log.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef CORE_LOG_HH_
#define CORE_LOG_HH_

#include <core/output_buffer.hh>


class log : public output_buffer
{
public:
	log(u32 target=0);
	~log();
};

void log_install(int target, io_node* node);


#endif  // CORE_LOG_HH_

