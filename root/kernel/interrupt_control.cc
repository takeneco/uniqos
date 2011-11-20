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
	global_vars::gv.intr_ctl_obj->call_interrupt(index);
}


cause::stype intr_ctl::init()
{
	return cause::OK;
}

cause::stype
intr_ctl::add_handler(arch::intr_vec vec, interrupt_handler* h)
{
	if (vec > arch::INTR_UPPER)
		return cause::INVALID_PARAMS;

	if (!h || !h->handler)
		return cause::INVALID_PARAMS;

	handler_table[vec].handler_chain.insert_head(h);

	return cause::OK;
}

cause::stype
intr_ctl::set_post_handler(arch::intr_vec vec, post_intr_handler h)
{
	handler_table[vec].post_handler = h;

	return cause::OK;
}

#include "log.hh"


void intr_ctl::call_interrupt(u32 vector)
{
	intr_task& it = handler_table[vector];
	intr_handler_chain& ihc = it.handler_chain;
	for (interrupt_handler* ih = ihc.head();
	     ih;
	     ih = ihc.next(ih))
	{
		ih->handler(ih->param);
	}

	if (it.post_handler)
		handler_table[vector].post_handler();
}

