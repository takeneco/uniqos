/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GLOBAL_VARS_HH_
#define ARCH_X86_64_INCLUDE_GLOBAL_VARS_HH_

#include <arch.hh> // cpu_id
#include <config.h>


class cpu_ctl_common;
class irq_ctl;
class cpu_node;
class page_pool;
class gv_page;
namespace arch {
class page_ctl;
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
	gv_page*           gv_page_obj;
	arch::page_ctl*    page_ctl_obj;

	memcache_control*  memcache_ctl;
	core_class*      core;
	event_queue*     event_ctl_obj;

	resource_spec*     rc_spec;
	void* bootinfo;
};


extern _vars gv;

struct _arch
{
	cpu_ctl_common*    cpu_ctl_common_obj;
	irq_ctl*           irq_ctl_obj;
};

extern _arch arch;

struct _core
{
	/// page_pool_objs のエントリ数
	int                page_pool_cnt;

	page_pool**        page_pool_objs;

	mempool_ctl*       mempool_ctl_obj;
	intr_ctl*          intr_ctl_obj;

	/// カーネルが認識している CPU の数
	/// @note システムに搭載されている CPU の数ではない
	cpu_id             cpu_node_cnt;

	cpu_node*          cpu_node_objs[CONFIG_MAX_CPUS];
};

extern _core core;

}  // namespace global_vars


#endif  // include guard
