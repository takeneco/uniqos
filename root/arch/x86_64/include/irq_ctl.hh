/// @file  irq_ctl.hh
/// @brief control IRQ number.
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_IRQ_CONTROL_HH_
#define ARCH_X86_64_INCLUDE_IRQ_CONTROL_HH_

#include <ioapic.hh>


class irq_ctl
{
	ioapic_ctl ioapic;

public:
	cause::type init();
	cause::type interrupt_map(u32 irq, u32* intr_vec);
};

namespace arch {

cause::type irq_interrupt_map(u32 irq, u32* intr_vec);

}  // namespace arch


#endif  // include guard

