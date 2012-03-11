/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GLOBAL_VARS_HH_
#define ARCH_X86_64_INCLUDE_GLOBAL_VARS_HH_


class cpu_share;
class processor;
class core_page;
class page_ctl;
namespace arch {
class irq_ctl;
}  // namespace arch
class intr_ctl;
class memcache_control;
class mempool_ctl;
class core_class;
class event_queue;
class resource_spec;


namespace global_vars {


struct _vars
{
	cpu_share*         cpu_share_obj;
	processor*       logical_cpu_obj_array;
	core_page*       core_page_obj;
	page_ctl*        page_ctl_obj;
	mempool_ctl*     mempool_ctl_obj;
	arch::irq_ctl*   irq_ctl_obj;
	intr_ctl*        intr_ctl_obj;

	memcache_control*  memcache_ctl;
	core_class*      core;
	event_queue*     event_ctl_obj;

	resource_spec*     rc_spec;
	void* bootinfo;
};


extern _vars gv;

}  // namespace global_vars


#endif  // include guard
