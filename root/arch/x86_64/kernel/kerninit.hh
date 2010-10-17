/// @author  KATO Takeshi
//
// (C) 2010 KATO Takeshi

#ifndef ARCH_X86_64_KERNEL_KERNINIT_HH_
#define ARCH_X86_64_KERNEL_KERNINIT_HH_

#include "btypes.hh"


int cpu_init();

typedef void (*intr_handler)();

void intr_init();
void intr_set_handler(int intr, intr_handler hander);

u64 convert_vadr_to_padr(const void* vadr);

namespace arch {
namespace pmem {

cause::stype init();
cause::stype alloc_l1page(uptr* padr);

}  // namespace pmem
}  // namespace arch


#endif  // Include guard.

