/// @file   cpu.cc
/// @brief  basic_cpu class implementation.
//
// (C) 2012 KATO Takeshi
//

#include <cpu.hh>

#include <native_ops.hh>


cause::stype basic_cpu::init()
{
	return thrdctl.init();
}

void basic_cpu::run_intr_event()
{
	native::cli();

	for (;;) {
		event_item* ev = get_next_intr_event();
		if (!ev)
			break;

		ev->handler(ev->param);
	}

	native::sti();
}

/// @brief 外部割込みからのイベントを登録する。
//
/// 外部割込み中は CPU が割り込み禁止状態なので割り込み可否の制御はしない。
void basic_cpu::post_intr_event(event_item* ev)
{
	intr_evq.push(ev);
}

/// @brief 外部割込みで登録されたイベントを返す。
event_item* basic_cpu::get_next_intr_event()
{
	return intr_evq.pop();
}

