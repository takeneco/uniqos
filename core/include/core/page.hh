// @file   include/page.hh
// @brief  Memory page alloc/free.
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_PAGE_HH_
#define CORE_INCLUDE_PAGE_HH_

#include <arch.hh>


cause::type page_alloc(arch::page::TYPE page_type, uptr* padr);
cause::type page_alloc(cpu_id cpuid, arch::page::TYPE page_type, uptr* padr);
cause::type page_dealloc(arch::page::TYPE page_type, uptr padr);


#endif  // CORE_INCLUDE_PAGE_HH_
