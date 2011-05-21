/// @file  irq_control.cc
/// @brief control irq to interrupt map.
/// (for IOAPIC)
//
// (C) 2011 KATO Takeshi
//

#include "base_types.hh"
#include "core_class.hh"
#include "global_variables.hh"
#include "interrupt_control.hh"
#include "ioapic.hh"
#include "irq_control.hh"


namespace arch {

cause::stype irq_control::init()
{
	return ioapic.init_detect();
}

cause::stype irq_control::interrupt_map(u32 irq, u32* intr_vec)
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

	*intr_vec = vec;

	return cause::OK;
}

cause::stype irq_init()
{
	return global_variable::gv.core->irq_ctrl.init();
}

cause::stype irq_interrupt_map(u32 irq, u32* intr_vec)
{
	return global_variable::gv.core->irq_ctrl.interrupt_map(irq, intr_vec);
}

}  // namespace arch

extern "C" void on_irq()
{
	on_interrupt();

	arch::lapic_eoi();
}

