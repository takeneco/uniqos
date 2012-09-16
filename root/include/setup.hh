/// @file  setup.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_SETUP_HH_
#define INCLUDE_SETUP_HH_

#include <basic.hh>


cause::type mempool_init();
cause::type mem_io_setup();
cause::type intr_setup();
cause::type timer_setup();
cause::type timer_setup_cpu();


#endif  // include guard

