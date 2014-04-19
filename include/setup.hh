/// @file  core/include/core/setup.hh
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_SETUP_HH_
#define CORE_INCLUDE_CORE_SETUP_HH_

#include <core/basic.hh>


cause::t mempool_init();
cause::t mempool_post_setup();
cause::t log_init();
cause::t mem_io_setup();
cause::t intr_setup();
cause::t timer_setup();


#endif  // include guard

