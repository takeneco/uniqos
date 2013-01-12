/// @file  setup.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_SETUP_HH_
#define INCLUDE_SETUP_HH_

#include <basic.hh>


cause::type mempool_init();
cause::type mempool_post_setup();
cause::type log_init();
cause::type mem_io_setup();
cause::type intr_setup();
cause::type timer_setup();


#endif  // include guard

