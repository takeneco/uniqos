/// @file   interrupt_control.cc
/// @brief  interrupt.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch_specs.hh"
#include "core_class.hh"
#include "global_vars.hh"
#include "interrupt_control.hh"


/// @brief 割り込み発生時に呼ばれる。
/// @param vec 割り込みベクタ。
extern "C" void on_interrupt(arch::intr_vec index)
{
	global_vars::gv.core->intr_ctrl.call_interrupt(index);
}


cause::stype interrupt_control::init()
{
	return cause::OK;
}

cause::stype
interrupt_control::add_handler(arch::intr_vec vec, interrupt_handler* h)
{
	if (vec > arch::INTR_UPPER)
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

#include "log.hh"


void interrupt_control::call_interrupt(u32 vector)
{
	intr_task& it = handler_table[vector];
	intr_handler_chain& ihc = it.handler_chain;
	for (interrupt_handler* ih = ihc.get_head();
	     ih;
	     ih = ihc.get_next(ih))
	{
		ih->handler(ih->param);
	}

	if (it.post_handler)
		handler_table[vector].post_handler();
}

