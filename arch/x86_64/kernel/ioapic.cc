/// @file  ioapic.cc
/// @brief I/O APIC contorl.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <ioapic.hh>

#include <arch.hh>
#include <cpu_ctl.hh>
#include <global_vars.hh>
#include <log.hh>


cause::t ioapic_ctl::init_detect()
{
	const mpspec* mps = global_vars::arch.cpu_ctl_common_obj->get_mpspec();
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

