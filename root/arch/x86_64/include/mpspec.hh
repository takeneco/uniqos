/// @file   mpspec.hh
/// @brief  declare of multi processor specification.
///
/// MultiProcessor Specification:
/// http://developer.intel.com/design/pentium/datashts/242016.htm
//
// (C) 2010 KATO Takeshi
//

#include "base_types.hh"


namespace mpspec {

/// MP floating pointer structure
struct mp_floating_pointer_structure
{
	u32 signature;       ///< must be "_MP_"
	u32 mp_config_padr;  ///< MP configuration table
	u8  length;          ///< =16
	u8  spec_rev;        ///< spec version
	u8  checksum;
	u8  features1;
	u8  features2;
	u8  features3[3];

	bool test() const;
};
typedef const mp_floating_pointer_structure const_mpfps;

/// MP configuration table header
struct mp_configuration_table_header
{
	u32  signature;          ///< must be "PCMP"
	u16  base_table_length;  ///< table and header size
	u8   spec_rev;           ///< spec version
	u8   checksum;
	char oem_id[8];
	char product_id[12];
	u32  oem_table_padr;
	u16  oem_table_size;
	u16  entry_count;
	u32  local_apic_map_padr;
	u16  ex_table_length;
	u8   ex_table_checksum;
	u8   reserved;

	bool test() const;
	const u8* first_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};
typedef const mp_configuration_table_header const_mpcth;

enum {
	ENTRY_PROCESSOR = 0,
	ENTRY_BUS,
	ENTRY_IOAPIC,
	ENTRY_IOINT,
	ENTRY_LOCALINT,
};

/// Processor entry
struct processor_entry
{
	u8  entry_type;  ///< =0
	u8  localapic_id;
	u8  localapic_version;
	u8  cpu_flags;
	u32 cpu_signature;
	u32 feature_flags;
	u8  reserved[8];

	const u8* next_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};


/// Bus entry
struct bus_entry
{
	u8  entry_type;  ///< =1
	u8  bus_id;
	u8  bus_type[6];

	const u8* next_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};


/// I/O APIC entry
struct ioapic_entry
{
	u8  entry_type;  ///< =2
	u8  ioapic_id;
	u8  ioapic_version;
	u8  ioapic_flags;
	u32 ioapic_map_padr;

	const u8* next_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};


/// I/O interrupt assignment entry
struct ioint_entry
{
	u8  entry_type;  ///< =3
	u8  int_type;
	u16 ioint_flags;
	u8  source_bus_id;
	u8  source_bus_irq;
	u8  dest_ioapic_id;
	u8  dest_ioapic_intin;

	const u8* next_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};


/// Local interrupt assignment entry
struct localint_entry
{
	u8  entry_type;  ///< =4
	u8  int_type;
	u16 localint_flags;
	u8  source_bus_id;
	u8  source_bus_irq;
	u8  dest_localapic_id;
	u8  dest_localapic_intin;

	const u8* next_entry() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};

const_mpfps* scan_mpfps(const void* start_adr, u32 length);

}  // namespace mpspec

