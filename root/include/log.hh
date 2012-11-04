/// @file  log.hh
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef INCLUDE_LOG_HH_
#define INCLUDE_LOG_HH_

#include <output_buffer.hh>


class log : public output_buffer
{
public:
	log(u32 target=0);
	~log();
};

void log_install(int target, io_node* node);


#endif  // include guard

