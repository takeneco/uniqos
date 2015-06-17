// @file   include/page.hh
// @brief  Memory page alloc/free.
//
// (C) 2012-2015 KATO Takeshi
//

#ifndef CORE_PAGE_HH_
#define CORE_PAGE_HH_

#include <core/pagetbl.hh>


cause::pair<uptr> page_alloc(page_level page_type);
cause::pair<uptr> page_alloc(cpu_id cpuid, page_level page_type);

cause::t page_alloc(page_level page_type, uptr* padr);
cause::t page_alloc(cpu_id cpuid, page_level page_type, uptr* padr);
cause::t page_dealloc(page_level page_type, uptr padr);


#endif  // include guard

