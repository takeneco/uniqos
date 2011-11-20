/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_
#define ARCH_X86_64_INCLUDE_GLOBAL_VARIABLES_HH_


class core_page;
class page_ctl;
namespace arch {
class irq_ctl;
}  // namespace arch
class memcache_control;
class core_class;
class event_queue;


namespace global_vars {


struct _vars
{
	core_page*       core_page_obj;
	page_ctl*        page_ctl_obj;
	arch::irq_ctl*   irq_ctl_obj;

	memcache_control*  memcache_ctl;
	core_class*      core;
	event_queue*     events;
};


extern _vars gv;

}  // namespace global_vars


#endif  // include guard
