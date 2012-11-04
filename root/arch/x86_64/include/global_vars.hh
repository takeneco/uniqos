/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_GLOBAL_VARS_HH_
#define INCLUDE_GLOBAL_VARS_HH_

#include <arch.hh> // cpu_id
#include <config.h>


class cpu_ctl_common;
class irq_ctl;

class page_pool;
class mempool_ctl;
class intr_ctl;
class timer_ctl;
class log_target;
class cpu_node;


namespace global_vars {

struct _arch
{
	cpu_ctl_common*    cpu_ctl_common_obj;

	irq_ctl*           irq_ctl_obj;

	void*              bootinfo;
};

extern _arch arch;

struct _core
{
	/// メモリに書き出すログのバッファ
	void*              memlog_buffer;

	/// page_pool_objs のエントリ数
	int                page_pool_cnt;

	/// log_target_objs のエントリ数
	int                log_target_cnt;

	page_pool**        page_pool_objs;

	mempool_ctl*       mempool_ctl_obj;

	intr_ctl*          intr_ctl_obj;

	timer_ctl*         timer_ctl_obj;

	log_target*        log_target_objs;

	/// カーネルが認識している CPU の数
	/// @note システムに搭載されている CPU の数ではない
	cpu_id             cpu_node_cnt;

	cpu_node*          cpu_node_objs[CONFIG_MAX_CPUS];
};

extern _core core;

}  // namespace global_vars


#endif  // include guard

