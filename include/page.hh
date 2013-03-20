// @file   include/page.hh
// @brief  Memory page alloc/free.
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_PAGE_HH_
#define INCLUDE_PAGE_HH_

#include <arch.hh>


cause::type page_alloc(arch::page::TYPE page_type, uptr* padr);
cause::type page_alloc(cpu_id cpuid, arch::page::TYPE page_type, uptr* padr);
cause::type page_dealloc(arch::page::TYPE page_type, uptr padr);


#endif  // include guard
