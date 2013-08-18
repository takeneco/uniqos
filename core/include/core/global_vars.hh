/// @file  global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_GLOBAL_VARS_HH_
#define CORE_INCLUDE_CORE_GLOBAL_VARS_HH_

#include <arch.hh> // cpu_id
#include <config.h>


class page_pool;
class mempool_ctl;
class intr_ctl;
class timer_ctl;
class log_target;
class cpu_node;

namespace global_vars {

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

