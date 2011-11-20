/// @file  irq_control.cc
/// @brief control irq to interrupt map.
/// (for IOAPIC)
//
// (C) 2011 KATO Takeshi
//

#include "base_types.hh"
#include "core_class.hh"
#include "global_vars.hh"
#include "interrupt_control.hh"
#include "ioapic.hh"
#include "irq_control.hh"


namespace arch {

cause::stype irq_ctl::init()
{
	cause::stype r = ioapic.init_detect();
	if (r != cause::OK)
		return r;

	// TODO:シリアルコントローラを初期化する前に割り込みが入るため、
	// EOI する必要がある。
	// 初期化前の割り込みが無ければ、このコードは削除できる。
	global_vars::gv.core->intr_ctrl.set_post_handler(0x5e, lapic_eoi);
	global_vars::gv.core->intr_ctrl.set_post_handler(0x5f, lapic_eoi);

	return r;
}

cause::stype irq_ctl::interrupt_map(u32 irq, u32* intr_vec)
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
	global_vars::gv.core->intr_ctrl.set_post_handler(vec, lapic_eoi);

	*intr_vec = vec;

	return cause::OK;
}

cause::stype irq_interrupt_map(u32 irq, u32* intr_vec)
{
	return global_vars::gv.irq_ctl_obj->interrupt_map(irq, intr_vec);
}

}  // namespace arch

