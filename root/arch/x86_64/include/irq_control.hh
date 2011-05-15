/// @file  irq_control.hh
/// @brief control IRQ number.
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_IRQ_CONTROL_HH_
#define ARCH_X86_64_INCLUDE_IRQ_CONTROL_HH_

#include "ioapic.hh"


namespace arch {

class irq_control
{
	ioapic_control ioapic;

public:
	cause::stype init();
	cause::stype interrupt_map(u32 irq, u32* intr_vec);
};

cause::stype irq_interrupt_map(u32 irq, u32* intr_vec);

}  // namespace arch


#endif  // include guard

