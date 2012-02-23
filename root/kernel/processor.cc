/// @file   cpu.cc
/// @brief  processor class implementation.
//
// (C) 2012 KATO Takeshi
//

#include <processor.hh>

#include <arch.hh>

#include <native_ops.hh>


cause::stype processor::init()
{
	cause::stype r = arch::cpu_ctl::init();
	if (is_fail(r))
		return r;

	r = thrdctl.init();
	if (is_fail(r))
		return r;

	arch::cpu_ctl::set_running_thread(thrdctl.get_running_thread());

	return cause::OK;
}

void processor::run_intr_event()
{
	for (;;) {
		preempt_disable();

		event_item* ev = get_next_intr_event();

		preempt_enable();

		if (!ev)
			break;

		ev->handler(ev->param);
	}
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void processor::post_intr_event(event_item* ev)
{
	intr_evq.push(ev);
}

void processor::post_soft_event(event_item* ev)
{
	soft_evq.push(ev);
}

/// @brief 外部割込みで登録されたイベントを返す。
event_item* processor::get_next_intr_event()
{
	return intr_evq.pop();
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

