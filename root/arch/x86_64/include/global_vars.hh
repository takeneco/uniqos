/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_
#define ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_


class page_control;
class core_class;
class event_queue;
class memcache_ctrl;


namespace global_vars {


struct _vars
{
	page_control*    page_ctl;
	core_class*      core;
	event_queue*     events;
	memcache_ctrl*   memcache_ctrl_;
};


extern _vars gv;

}  // namespace global_vars


#endif  // include guard
