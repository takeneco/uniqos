/// @file  ioapic.cc
/// @brief I/O APIC contorl.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "log.hh"
#include "mpspec.hh"


namespace {

mpspec::const_mpfps* search_mpfps()
{
	mpspec::const_mpfps* mpfps;

	const uptr ebda =
	    *reinterpret_cast<u16*>(arch::pmem::direct_map(0x40e)) << 4;
	mpfps = mpspec::scan_mpfps(arch::pmem::direct_map(ebda), 0x400);
	if (mpfps)
		return mpfps;

	mpfps = mpspec::scan_mpfps(arch::pmem::direct_map(0xf0000), 0x10000);
	if (mpfps)
		return mpfps;

	mpfps = mpspec::scan_mpfps(arch::pmem::direct_map(0x9fc00), 0x400);
	if (mpfps)
		return mpfps;

	return 0;
}

const mpspec::ioapic_entry* search_ioapic(mpspec::const_mpfps* mpfps)
{
	mpspec::const_mpcth* mpcth = reinterpret_cast<mpspec::const_mpcth*>(
	    arch::pmem::direct_map(mpfps->mp_config_padr));

	static const u16 type_size_map[] = {
		sizeof (mpspec::processor_entry),
		sizeof (mpspec::bus_entry),
		sizeof (mpspec::ioapic_entry),
		sizeof (mpspec::ioint_entry),
		sizeof (mpspec::localint_entry),
	};

	const u8* ent = mpcth->first_entry();
	for (int i = 0; i < mpcth->entry_count; ++i) {
		if (*ent == mpspec::ENTRY_IOAPIC)
			return
			    reinterpret_cast<const mpspec::ioapic_entry*>(ent);
		if (*ent >= 5)
			break;
		ent += type_size_map[*ent];
	}

	return 0;
}

}  // namespace

namespace arch {

void* ioapic_base()
{
	mpspec::const_mpfps* mpfps = search_mpfps();
	if (mpfps == 0) {
		log()("MPFPS not found.")();
		return 0;
	}

	const mpspec::ioapic_entry* ioapic_etr = search_ioapic(mpfps);
	if (ioapic_etr == 0) {
		log()("I/O APIC not found.")();
		return 0;
	}

	return arch::pmem::direct_map(ioapic_etr->ioapic_map_padr);
}

}  // namespace arch


