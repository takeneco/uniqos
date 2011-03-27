/// @file   mpspec.cc
/// @brief  multi processor specification implements.
///
/// MultiProcessor Specification:
/// http://developer.intel.com/design/pentium/datashts/242016.htm
//
// (C) 2010 KATO Takeshi
//

#include "mpspec.hh"


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


namespace mpspec {

/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mp_floating_pointer_structure::test() const
{
	if (signature != le32_to_cpu('_', 'M', 'P', '_'))
		return false;

	return sum8(this, sizeof *this) == 0;
}


/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mp_configuration_table_header::test() const
{
	if (signature != le32_to_cpu('P', 'C', 'M', 'P'))
		return false;

	return sum8(this, base_table_length) == 0;
}


/// @return  returns ptr to mpfps.
/// @return  if floating ptr not found returns 0.
const_mpfps* scan_mpfps(const void* start_adr, u32 length)
{
	const u8* base = reinterpret_cast<const u8*>(start_adr);
	const u32 end = length - sizeof (mp_floating_pointer_structure) + 1;
	for (u32 i = 0; i <= end; i += 4) {
		const_mpfps* p = reinterpret_cast<const_mpfps*>(base + i);
		if (p->test())
			return p;
	}

	return 0;
}

}  // namespace mpspec

