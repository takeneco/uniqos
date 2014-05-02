/// @file   intr_ctl.cc
/// @brief  Interrupt control.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2014 KATO Takeshi
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

#include <core/global_vars.hh>
#include <core/mempool.hh>
#include <intr_ctl.hh>
#include <new_ops.hh>


/// @brief 割り込み発生時に呼ばれる。
/// @param vec 割り込みベクタ。
extern "C" void on_interrupt(arch::intr_id index)
{
	global_vars::core.intr_ctl_obj->call_interrupt(index);
}


cause::t intr_ctl::init()
{
	return cause::OK;
}

cause::t intr_ctl::install_handler(arch::intr_id vec, intr_handler* h)
{
	if (vec > arch::INTR_UPPER)
		return cause::INVALID_PARAMS;

	if (!h || !h->handler)
		return cause::INVALID_PARAMS;

	handler_table[vec].handler_chain.insert_head(h);

	return cause::OK;
}

cause::t intr_ctl::set_post_handler(arch::intr_id vec, post_intr_handler h)
{
	handler_table[vec].post_handler = h;

	return cause::OK;
}

void intr_ctl::call_interrupt(u32 vector)
{
	intr_task& it = handler_table[vector];
	intr_handler_chain& ihc = it.handler_chain;
	for (intr_handler* ih = ihc.head(); ih; ih = ihc.next(ih))
	{
		ih->handler(ih);
	}

	if (it.post_handler)
		handler_table[vector].post_handler();
}

cause::t intr_setup()
{
	intr_ctl* intrc = new (mem_alloc(sizeof (intr_ctl))) intr_ctl;
	if (!intrc)
		return cause::NOMEM;

	global_vars::core.intr_ctl_obj = intrc;

	cause::t r = intrc->init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

