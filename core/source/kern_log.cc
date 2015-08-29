/// @file  kern_log.cc

//  Uniqos  --  Unique Operating System
//  (C) 2011 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

/// @todo log を生成するときに offset を読み出し、破棄するときに
///       offset を書き出しているので、並列出力すると offset が壊れる。

#include <core/log.hh>

#include <core/global_vars.hh>
#include <core/mempool.hh>
#include <core/log_target.hh>


/// @pre mem_alloc() が使用できること。つまり mempool_init() が済んでいること。
cause::t log_init()
{
	const int obj_cnt = 3;

	cause::t r = log_target::setup();
	if (is_fail(r))
		return r;

	log_target* objs = new (generic_mem()) log_target[obj_cnt];

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

