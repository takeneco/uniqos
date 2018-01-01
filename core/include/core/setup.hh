/// @file  core/setup.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef CORE_SETUP_HH_
#define CORE_SETUP_HH_

#include <core/basic.hh>


cause::t mempool_setup();
cause::t mempool_post_setup();
cause::t log_init();
cause::t mem_io_setup();
cause::t intr_setup();
cause::t ns_setup();
cause::t device_ctl_setup();
cause::t devnode_setup();
cause::t driver_ctl_setup();
cause::t timer_setup();
cause::t module_ctl_init();
cause::t fs_ctl_init();
cause::t vadr_pool_setup();
cause::t ata_setup();
cause::t ahci_setup();
cause::t pci_setup();


#endif  // CORE_SETUP_HH_

