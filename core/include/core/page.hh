// @file   include/page.hh
// @brief  Memory page alloc/free.
//
// (C) 2012-2015 KATO Takeshi
//

#ifndef CORE_PAGE_HH_
#define CORE_PAGE_HH_

#include <arch/pagetable.hh>


cause::t page_alloc(arch::page::TYPE page_type, uptr* padr);
cause::t page_alloc(cpu_id cpuid, arch::page::TYPE page_type, uptr* padr);
cause::t page_dealloc(arch::page::TYPE page_type, uptr padr);


#endif  // include guard

