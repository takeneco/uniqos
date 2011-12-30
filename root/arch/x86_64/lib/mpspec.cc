/// @file   mpspec.cc
/// @brief  multi processor specification implements.
///
/// MultiProcessor Specification:
/// http://developer.intel.com/design/pentium/datashts/242016.htm
//
// (C) 2010-2011 KATO Takeshi
//

#include "mpspec.hh"

#include "arch.hh"


namespace {

/// checksum
// duplicated
u8 sum8(const void* ptr, u32 length)
{
	const u8* p = reinterpret_cast<const u8*>(ptr);
	u8 sum = 0;
	for (u32 i = 0; i < length; ++i)
		sum += p[i];

	return sum;
}

}  // namespace


/// @retval cause::OK  Succeeds.
/// @retval cause::NOT_FOUND  MP specification not found.
cause::stype mpspec::load()
{
	return search(&mpfps, &mpcth);
}

/// @brief  MPFPS を探す。
cause::stype mpspec::search(
    const_mpfps** r_mpfps,  ///< [out] ptr to mpfps.
    const_mpcth** r_mpcth)  ///< [out] ptr to mpcth.
{
	const u16* bda = static_cast<u16*>(arch::map_phys_adr(0x400, 0x100));
	volatile u32 ebda_seg = bda[7];  // physical address : 0x40e
	arch::unmap_phys_adr(bda, 0x100);

	cause::stype r = scan_mpfps(ebda_seg << 4, 0x400, r_mpfps, r_mpcth);
	if (is_ok(r))
		return r;

	r = scan_mpfps(0xf0000, 0x10000, r_mpfps, r_mpcth);
	if (is_ok(r))
		return r;

	r = scan_mpfps(0x9fc00, 0x400, r_mpfps, r_mpcth);
	if (is_ok(r))
		return r;

	// mpfps が必ずあるとは限らないらしい。
	return cause::NOT_FOUND;
}

/// @brief  メモリをスキャンして MPFPS を探す。
//
/// MPFPS を見つけたときは、そのアドレスと MPCTH を arch::map_phys_adr()
/// でマップしたポインタを返す。
/// @retval cause::OK        MPFPS found.
/// @retval cause::NOT_FOUND MPFPS not found.
cause::stype mpspec::scan_mpfps(
    u32 start_padr,         ///< [in] physical address of scan target.
    u32 bytes,              ///< [in] scan bytes.
    const_mpfps** r_mpfps,  ///< [out] ptr to mpfsp.
    const_mpcth** r_mpcth)  ///< [out] ptr to mpcth.
{
	const u8* base =
	    static_cast<u8*>(arch::map_phys_adr(start_padr, bytes));

	const s32 end = bytes - sizeof (const_mpfps);
	for (s32 i = 0; i <= end; i += 4) {
		const_mpfps* mpfps = reinterpret_cast<const_mpfps*>(base + i);
		if (mpfps->test() && is_ok(test_mpcth(mpfps, r_mpcth))) {
			*r_mpfps = mpfps;
			return cause::OK;
		}
	}

	arch::unmap_phys_adr(base, bytes);

	return cause::NOT_FOUND;
}

/// @brief  Call MPCTH->test().
/// @retval cause::OK test() is succeeds.
/// @return cause::FAIL test() is failed.
cause::stype mpspec::test_mpcth(
    const_mpfps* mpfps,     ///< [in] ptr to mpfps.
    const_mpcth** r_mpcth)  ///< [out] ptr to mpcth.
{
	const_mpcth* mpcth = static_cast<const_mpcth*>(
	    arch::map_phys_adr(mpfps->mp_config_padr, sizeof (const_mpcth)));

	volatile const u16 sizeof_mpcth = mpcth->base_table_length;

	arch::unmap_phys_adr(mpcth, sizeof (const_mpcth));

	mpcth = static_cast<const_mpcth*>(
	    arch::map_phys_adr(mpfps->mp_config_padr, sizeof_mpcth));

	if (mpcth->test()) {
		*r_mpcth = mpcth;
		return cause::OK;
	}

	arch::unmap_phys_adr(mpcth, sizeof_mpcth);

	return cause::FAIL;
}


// mpspec::mp_floating_pointer_structure


/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mpspec::mp_floating_pointer_structure::test() const
{
	if (signature != le32_to_cpu('_', 'M', 'P', '_'))
		return false;

	return sum8(this, sizeof *this) == 0;
}


// mpspec::mp_configuration_table_header


/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mpspec::mp_configuration_table_header::test() const
{
	if (signature != le32_to_cpu('P', 'C', 'M', 'P'))
		return false;

	return sum8(this, base_table_length) == 0;
}


// mpspec::iterator


mpspec::iterator::iterator(const_mpcth* mpcth)
:   entry(mpcth->first_entry()),
    max_count(mpcth->entry_count),
    count(0)
{}

const void* mpspec::iterator::get_next()
{
	if (count == max_count)
		return 0;

	const void* r = entry;

	switch (*entry) {
	case PROCESSOR:
		entry += sizeof (processor_entry);
		break;
	case BUS:
		entry += sizeof (bus_entry);
		break;
	case IOAPIC:
		entry += sizeof (ioapic_entry);
		break;
	case IOINTR:
		entry += sizeof (iointr_entry);
		break;
	case LOCALINTR:
		entry += sizeof (localintr_entry);
		break;
	}

	++count;

	return r;
}

