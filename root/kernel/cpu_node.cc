/// @file   cpu_node.cc
/// @brief  cpu_node class implementation.
//
// (C) 2012 KATO Takeshi
//

#include <processor.hh>

#include <arch.hh>
#include <global_vars.hh>

#include <native_ops.hh>


/// @brief  page_pool を指定する。
cause::type cpu_node::set_page_pool(int pri, page_pool* pp)
{
	page_pools[pri] = pp;

	return cause::OK;
}

cause::type cpu_node::init()
{
	preempt_disable_nests = 0;

	cause::type r = arch::cpu_ctl::init();
	if (is_fail(r))
		return r;

	r = thrdctl.init();
	if (is_fail(r))
		return r;

	arch::cpu_ctl::set_running_thread(thrdctl.get_running_thread());

	return cause::OK;
}

/// @brief 外部割込みからのイベントをすべて処理する。
//
/// 割り込み禁止状態で呼び出す必要がある。
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

void cpu_node::preempt_disable()
{
#if CONFIG_PREEMPT
	arch::intr_disable();
	++preempt_disable_nests;
#endif  // CONFIG_PREEMPT
}

void cpu_node::preempt_enable()
{
#if CONFIG_PREEMPT
	if (--preempt_disable_nests == 0)
		arch::intr_enable();
#endif  // CONFIG_PREEMPT
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void cpu_node::post_intr_event(event_item* ev)
{
	intr_evq.push(ev);
}

void cpu_node::post_soft_event(event_item* ev)
{
	soft_evq.push(ev);
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


int get_cpu_node_count()
{
	return global_vars::gv.cpu_node_cnt;
}

cpu_node* get_cpu_node()
{
	const int id = arch::get_cpu_id();

	return global_vars::gv.cpu_node_objs[id];
}

cpu_node* get_cpu_node(int cpuid)
{
	return global_vars::gv.cpu_node_objs[cpuid];
}

cpu_node* get_current_cpu()
{
	return &global_vars::gv.logical_cpu_obj_array[0];
}

/// Preemption contorl

void preempt_enable()
{
#if CONFIG_PREEMPT
	arch::intr_enable();
#endif  // CONFIG_PREEMPT
}

void preempt_disable()
{
#if CONFIG_PREEMPT
	arch::intr_disable();
#endif  // CONFIG_PREEMPT
}

