/// @file  ioapic.cc
/// @brief I/O APIC contorl.
//
// (C) 2011 KATO Takeshi
//

#include "ioapic.hh"

#include "arch.hh"
#include "cpu_ctl.hh"
#include "global_vars.hh"
#include "log.hh"


cause::stype ioapic_ctl::init_detect()
{
	const mpspec* mps = global_vars::gv.cpu_share_obj->get_mpspec();
	mpspec::ioapic_iterator itr(mps);

	const mpspec::ioapic_entry* ioapic_ent = itr.get_next();
	if (ioapic_ent == 0) {
		log()("I/O APIC not found.")();
		return cause::NOT_FOUND;
	}

	regs = static_cast<memmapped_regs*>(arch::map_phys_adr(
	    ioapic_ent->ioapic_map_padr, sizeof (memmapped_regs)));

	return cause::OK;
}

