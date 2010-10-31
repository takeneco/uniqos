/// @file  memory_allocate.hh
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_MEMORY_ALLOCATE_HH_
#define ARCH_X86_64_KERNEL_MEMORY_ALLOCATE_HH_

#include "btypes.hh"


namespace arch {

namespace pmem {

cause::stype init();
cause::stype alloc_l1page(uptr* padr);
cause::stype alloc_l2page(uptr* padr);
cause::stype free_l1page(uptr padr);
cause::stype free_l2page(uptr padr);

}  // namespace pmem

}  // namespace arch


#endif  // Include guard.

