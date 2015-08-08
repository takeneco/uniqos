/// @file  core/global_vars.hh
/// @brief Global variables declaration.
//
// (C) 2013-2015 KATO Takeshi
//

#ifndef CORE_GLOBAL_VARS_HH_
#define CORE_GLOBAL_VARS_HH_

#include <arch.hh> // cpu_id
#include <config.h>


class cpu_node;
class device_ctl;
class dev_node_ctl;
class driver_ctl;
class fs_ctl;
class intr_ctl;
class log_target;
class mempool_ctl;
class module_ctl;
class page_pool;
class process_ctl;
class timer_ctl;
class vadr_pool;

namespace global_vars {

struct _core
{
	/// メモリに書き出すログのバッファ
	void*              memlog_buffer;

	/// page_pool_objs のエントリ数
	u32                page_pool_nr;

	/// log_target_objs のエントリ数
	int                log_target_cnt;

	/// カーネルが認識している CPU の数
	/// @note システムに搭載されている CPU の数ではない
	cpu_id_t           cpu_node_nr;

	cpu_node*          cpu_node_objs[CONFIG_MAX_CPUS];

	device_ctl*        device_ctl_obj;

	dev_node_ctl*     dev_node_ctl_obj;

	driver_ctl*        driver_ctl_obj;

	fs_ctl*            fs_ctl_obj;

	intr_ctl*          intr_ctl_obj;

	log_target*        log_target_objs;

	mempool_ctl*       mempool_ctl_obj;

	module_ctl*        module_ctl_obj;

	page_pool**        page_pool_objs;

	process_ctl*       process_ctl_obj;

	timer_ctl*         timer_ctl_obj;

	vadr_pool*         vadr_pool_obj;
};

extern _core core;

}  // namespace global_vars


#endif  // CORE_GLOBAL_VARS_HH_

