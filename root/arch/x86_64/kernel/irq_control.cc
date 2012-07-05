/// @file  irq_ctl.cc
/// @brief control irq to interrupt map.
/// (for IOAPIC)

//  Uniqos  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <irq_ctl.hh>

#include <global_vars.hh>
#include <interrupt_control.hh>
#include <mempool.hh>
#include <new_ops.hh>


cause::type irq_ctl::init()
{
	cause::type r = ioapic.init_detect();
	if (is_fail(r))
		return r;

	// TODO:シリアルコントローラを初期化する前に割り込みが入るため、
	// EOI する必要がある。
	// 初期化前の割り込みが無ければ、このコードは削除できる。
	global_vars::core.intr_ctl_obj->set_post_handler(0x5e, lapic_eoi);
	global_vars::core.intr_ctl_obj->set_post_handler(0x5f, lapic_eoi);

	return r;
}

cause::type irq_ctl::interrupt_map(u32 irq, u32* intr_vec)
{
	u32 vec;

	switch (irq) {
	case 3: // COM2
		vec = 0x5f;
		break;
	case 4: // COM1
		vec = 0x5e;
		break;
	default:
		return cause::UNKNOWN;
	}

	ioapic.unmask(irq, 0, vec);
	global_vars::core.intr_ctl_obj->set_post_handler(vec, lapic_eoi);

	*intr_vec = vec;

	return cause::OK;
}

namespace arch {

cause::type irq_interrupt_map(u32 irq, u32* intr_vec)
{
	return global_vars::arch.irq_ctl_obj->interrupt_map(irq, intr_vec);
}

}  // namespace arch

cause::type irq_setup()
{
	irq_ctl* irqc =
	    new (mem_alloc(sizeof (irq_ctl))) irq_ctl;
	if (!irqc)
		return cause::NOMEM;

	global_vars::arch.irq_ctl_obj = irqc;

	cause::type r = irqc->init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

