/// @file   cpu_node.cc
/// @brief  cpu_node class implementation.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <cpu_node.hh>

#include <arch.hh>
#include <global_vars.hh>
#include <log.hh>
#include <page_pool.hh>

// TODO:include from arch
#include <native_thread.hh>
#include <native_ops.hh>

/// @class cpu_node
//
/// cpu_node の初期化方法
///
/// (1) コンストラクタを呼び出す。
///     割り込み制御(preempt_disable/enable)が可能になる。
/// (2) set_page_pool_cnt() と set_page_pool() で page_pool を設定する。
///     ページ割り当てが可能になる。mempool によるメモリ割り当ても可能。
/// (3) 各CPU から setup() を呼び出す。

cpu_node::cpu_node() :
	thread_q(this)
{
}

cause::type cpu_node::set_page_pool_cnt(int cnt)
{
	page_pool_cnt = cnt;

	for (cpu_id i = cnt; i < num_of_array(page_pools); ++i)
		page_pools[i] = 0;

	return cause::OK;
}

/// @brief  page_pool を指定する。
cause::type cpu_node::set_page_pool(int pri, page_pool* pp)
{
	page_pools[pri] = pp;

	return cause::OK;
}

cause::type cpu_node::setup()
{
	cause::t r = thread_q.init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

void cpu_node::preempt_wait()
{
	arch::intr_wait();
}

cause::type cpu_node::page_alloc(arch::page::TYPE page_type, uptr* padr)
{
	for (cpu_id i = 0; i < page_pool_cnt; ++i) {
		const cause::type r = page_pools[i]->alloc(page_type, padr);
		if (is_ok(r))
			return cause::OK;
	}

	return cause::FAIL;
}

cause::type cpu_node::page_dealloc(arch::page::TYPE page_type, uptr padr)
{
	for (cpu_id i = 0; i < page_pool_cnt; ++i) {
		const cause::type r = page_pools[i]->dealloc(page_type, padr);
		if (is_ok(r)) {
			return cause::OK;
		} else if (r == cause::OUTOFRANGE) {
			continue;
		} else {
			log()("!!! cpu_node::page_dealloc() failed. r=").u(r)();
			return r;
		}
	}

	return cause::FAIL;
}

cpu_id get_cpu_node_count()
{
	return global_vars::core.cpu_node_cnt;
}

cpu_node* get_cpu_node()
{
	const cpu_id id = arch::get_cpu_id();

	return global_vars::core.cpu_node_objs[id];
}

cpu_node* get_cpu_node(cpu_id cpuid)
{
	return global_vars::core.cpu_node_objs[cpuid];
}


/// Preemption contorl

preempt_disable_section::preempt_disable_section()
{
#if CONFIG_PREEMPT
	preempt_disable();

#endif  // CONFIG_PREEMPT
}

preempt_disable_section::~preempt_disable_section()
{
#if CONFIG_PREEMPT
	preempt_enable();

#endif  // CONFIG_PREEMPT
}

preempt_enable_section::preempt_enable_section()
{
#if CONFIG_PREEMPT
	preempt_enable();

#endif  // CONFIG_PREEMPT
}

preempt_enable_section::~preempt_enable_section()
{
#if CONFIG_PREEMPT
	preempt_disable();

#endif  // CONFIG_PREEMPT
}

void post_message(message* msg)
{
	arch::post_cpu_message(msg);
}

