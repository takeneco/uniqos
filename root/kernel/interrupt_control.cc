/// @file   interrupt_control.cc
/// @brief  interrupt.
//
// (C) 2010-2011 KATO Takeshi
//

#include "core_class.hh"
#include "global_variables.hh"
#include "interrupt_control.hh"


/// @brief 割り込み発生時に呼ばれる。
/// @param vec 割り込みベクタ。
extern "C" void on_interrupt(arch::intr_vec index)
{
	global_variable::gv.core->intr_ctrl.call_interrupt(index);
}


cause::stype interrupt_control::init()
{
	return cause::OK;
}

cause::stype
interrupt_control::add_handler(arch::intr_vec vec, interrupt_handler* h)
{
	if (vec >= 0x40)
		return cause::INVALID_PARAMS;

	if (!h || !h->handler)
		return cause::INVALID_PARAMS;

	handler_table[vec].handler_chain.insert_head(h);

	return cause::OK;
}

cause::stype
interrupt_control::set_post_handler(arch::intr_vec vec, post_intr_handler h)
{
	handler_table[vec].post_handler = h;

	return cause::OK;
}

extern "C" void interrupt_timer();

void interrupt_control::call_interrupt(u32 vector)
{
	intr_handler_chain& ihc = handler_table[vector].handler_chain;
	for (interrupt_handler* ih = ihc.get_head();
	     ih;
	     ih = ihc.get_next(ih))
	{
		ih->handler(ih->param);
	}

	interrupt_timer();
}

