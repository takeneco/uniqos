/// @file   mpspec.hh
/// @brief  declare of multi processor specification.
///
/// MultiProcessor Specification:
/// http://developer.intel.com/design/pentium/datashts/242016.htm
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_MPSPEC_HH_
#define ARCH_X86_64_INCLUDE_MPSPEC_HH_

#include "basic_types.hh"


class mpspec
{
public:
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

	enum ENTRY {
		PROCESSOR = 0,
		BUS = 1,
		IOAPIC = 2,
		IOINTR = 3,
		LOCALINTR = 4,
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
	};

	/// Bus entry
	struct bus_entry
	{
		u8  entry_type;  ///< =1
		u8  bus_id;
		u8  bus_type[6];
	};

	/// I/O APIC entry
	struct ioapic_entry
	{
		u8  entry_type;  ///< =2
		u8  ioapic_id;
		u8  ioapic_version;
		u8  ioapic_flags;
		u32 ioapic_map_padr;
	};

	/// I/O interrupt assignment entry
	struct iointr_entry
	{
		u8  entry_type;  ///< =3
		u8  intr_type;
		u16 iointr_flags;
		u8  source_bus_id;
		u8  source_bus_irq;
		u8  dest_ioapic_id;
		u8  dest_ioapic_intin;
	};

	/// Local interrupt assignment entry
	struct localintr_entry
	{
		u8  entry_type;  ///< =4
		u8  intr_type;
		u16 localintr_flags;
		u8  source_bus_id;
		u8  source_bus_irq;
		u8  dest_localapic_id;
		u8  dest_localapic_intin;
	};

	class iterator
	{
	public:
		iterator(const_mpcth* mpcth);

		u16 get_max_count() const { return max_count; }

	protected:
		const void* get_next();

	private:
		const u8* entry;
		const u16 max_count;
		u16       count;
	};

	template<class ENTRY> class entry_iterator : public iterator
	{
	public:
		explicit entry_iterator(const mpspec* mps)
		    : iterator(mps->get_mpcth()) {}
		const ENTRY* get_next() {
			return static_cast<const ENTRY*>(iterator::get_next());
		}
	};
	typedef entry_iterator<processor_entry> processor_iterator;
	typedef entry_iterator<bus_entry>       bus_iterator;
	typedef entry_iterator<ioapic_entry>    ioapic_iterator;
	typedef entry_iterator<iointr_entry>    iointr_iterator;
	typedef entry_iterator<localintr_entry> localintr_iterator;

public:
	mpspec() {}

	const_mpfps* get_mpfps() const { return mpfps; }
	const_mpcth* get_mpcth() const { return mpcth; }

	cause::stype load();

private:
	static cause::stype search(
	    const_mpfps** r_mpfps, const_mpcth** r_mpcth);
	static cause::stype scan_mpfps(
	    u32 start_padr, u32 length,
	    const_mpfps** r_mpfps, const_mpcth** r_mpcth);
	static cause::stype test_mpcth(
	    const_mpfps* mpfps, const_mpcth** r_mpcth);

private:
	const_mpfps* mpfps;
	const_mpcth* mpcth;
};


#endif  // include guard

