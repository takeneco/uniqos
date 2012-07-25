/// @file   cpu_node.cc
/// @brief  cpu_node class implementation.

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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


/// @class cpu_node
//
/// cpu_node の初期化方法
///
/// (1) コンストラクタを呼び出す。
///     割り込み制御が可能になる。
/// (2) set_page_pool_cnt() と set_page_pool() で page_pool を設定する。
///     ページ割り当てが可能になる。mempool によるメモリ割り当ても可能。
/// (3) 各CPU から setup() を呼び出す。

cpu_node::cpu_node() :
	preempt_disable_cnt(0),
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
	cause::type r = arch::cpu_ctl::setup();
	if (is_fail(r))
		return r;

	r = thread_q.init();
	if (is_fail(r))
		return r;

	arch::cpu_ctl::set_running_thread(thread_q.get_running_thread());

	return cause::OK;
}

cause::type cpu_node::start_message_loop()
{
	cause::type r = thread_q.create_thread(
	    &cpu_node::message_loop_entry, this, &message_thread);
	if (is_fail(r))
		return r;

	r = thread_q.wakeup(message_thread);
	if (is_fail(r))
		return r;

	return cause::OK;
}

void cpu_node::ready_messenger()
{
	thread_q.ready(message_thread);
}

void cpu_node::ready_messenger_np()
{
	thread_q.ready_np(message_thread);
}

void cpu_node::switch_messenger_after_intr()
{
	thread_q.switch_thread_after_intr(message_thread);
}

/// @brief 外部割込みからのイベントをすべて処理する。
/// @retval true  何らかのメッセージを処理した。
/// @retval false 処理するメッセージがなかった。
/// @note  preempt_disable の状態で呼び出す必要がある。
bool cpu_node::run_all_intr_event()
{
	if (!probe_intr_event())
		return false;

	do {
		event_item* ev = get_next_intr_event();

		ev->handler(ev->param);

	} while (probe_intr_event());

	return true;
}

void cpu_node::preempt_wait()
{
	arch::intr_wait();
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void cpu_node::post_intr_event(event_item* ev)
{
	intr_evq.push(ev);
	thread_q.ready_np(message_thread);
}

void cpu_node::post_soft_event(event_item* ev)
{
	preempt_enable();

	soft_evq.push(ev);
	thread_q.ready_np(message_thread);

	preempt_disable();
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
		if (is_ok(r))
			return cause::OK;
	}

	log()("!!! cpu_node::page_dealloc() failed.")();

	return cause::FAIL;
}

/// @retval true 外部割込みによって登録されたイベントがある。
bool cpu_node::probe_intr_event()
{
	return intr_evq.probe();
}

/// @brief 外部割込みで登録されたイベントを返す。
event_item* cpu_node::get_next_intr_event()
{
	return intr_evq.pop();
}

/// @brief  メッセージを１つだけ処理する。
/// @retval true  メッセージを処理した。
/// @retval false 処理するメッセージがなかった。
/// @note  preempt_disable の状態で呼び出す必要がある。
bool cpu_node::run_message()
{
	event_queue& evq = get_soft_evq();

	if (!evq.probe())
		return false;

	event_item* event = evq.pop();

	event->handler(event->param);

	return true;
}

void cpu_node::message_loop()
{
	for (;;) {
		arch::intr_enable();
		arch::intr_disable();

		if (run_all_intr_event())
			continue;

		if (run_message())
			continue;

		if (thread_q.force_switch_thread())
			continue;

		arch::intr_wait();
	}
}

void cpu_node::message_loop_entry(void* _cpu_node)
{
	static_cast<cpu_node*>(_cpu_node)->message_loop();
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

void preempt_disable()
{
#if CONFIG_PREEMPT

	arch::intr_disable();

	get_cpu_node()->inc_preempt_disable();

#endif  // CONFIG_PREEMPT
}

void preempt_enable()
{
#if CONFIG_PREEMPT

	if (get_cpu_node()->dec_preempt_disable() == 0)
		arch::intr_enable();

#endif  // CONFIG_PREEMPT
}

preempt_disable_section::preempt_disable_section()
{
#if CONFIG_PREEMPT

	arch::intr_disable();

	cn = get_cpu_node();

	cn->inc_preempt_disable();

#endif  // CONFIG_PREEMPT
}

preempt_disable_section::~preempt_disable_section()
{
#if CONFIG_PREEMPT

	if (cn->dec_preempt_disable() == 0)
	{
		arch::intr_enable();
	}

#endif  // CONFIG_PREEMPT
}

preempt_enable_section::preempt_enable_section()
{
#if CONFIG_PREEMPT

	cn = get_cpu_node();

	if (cn->dec_preempt_disable() == 0)
	{
		arch::intr_enable();
	}

#endif  // CONFIG_PREEMPT
}

preempt_enable_section::~preempt_enable_section()
{
#if CONFIG_PREEMPT

	arch::intr_disable();

	cn->inc_preempt_disable();

#endif  // CONFIG_PREEMPT
}

