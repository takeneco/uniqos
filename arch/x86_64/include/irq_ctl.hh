/// @file  irq_ctl.hh
/// @brief control IRQ number.
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_IRQ_CTL_HH_
#define ARCH_X86_64_INCLUDE_IRQ_CTL_HH_

#include <pic_dev.hh>
#include <ioapic.hh>


class irq_ctl
{
	ioapic_ctl ioapic;

public:
	cause::type init();
	cause::type interrupt_map(u32 irq, u32* intr_vec);

	cause::type set_pic(pic_device* pd) { pic_dev = pd; return cause::OK; }

	void _call_eoi() {
		pic_dev->info->ops.eoi(pic_dev);
	}

private:
	pic_device* pic_dev;
};

namespace arch {

cause::type irq_interrupt_map(u32 irq, u32* intr_vec);

}  // namespace arch


#endif  // include guard

