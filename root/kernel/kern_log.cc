/// @file  kern_log.cc
//
// (C) 2011-2012 KATO Takeshi
//

/// @todo log を生成するときに offset を読み出し、破棄するときに
///       offset を書き出しているので、並列出力すると offset が壊れる。

#include <log.hh>

#include <global_vars.hh>
#include <log_target.hh>
#include <mempool.hh>
#include <new_ops.hh>


/// @pre mem_alloc() が使用できること。つまり mempool_init() が済んでいること。
cause::type log_init()
{
	const int obj_cnt = 3;

	cause::type r = log_target::setup();
	if (is_fail(r))
		return r;

	void* mem = mem_alloc(sizeof (log_target[obj_cnt]));

	log_target* objs = new (mem) log_target[obj_cnt];

	if (!objs)
		return cause::NOMEM;

	global_vars::core.log_target_cnt = obj_cnt;
	global_vars::core.log_target_objs = objs;

	return cause::OK;
}

void log_install(int target, io_node* node)
{
	global_vars::core.log_target_objs[target].install(node, 0);
}

// log class

log::log(u32 target) :
	output_buffer(&global_vars::core.log_target_objs[target], 0)
{
}

log::~log()
{
	flush();
}

