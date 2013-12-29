/// @file  acpi_ctl.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef CORE_INCLUDE_ACPI_CTL_HH_
#define CORE_INCLUDE_ACPI_CTL_HH_

extern "C" {
#include <acpi.h>
}  // extern "C"

#include <basic.hh>


cause::type acpi_table_init(uptr size, void* buffer);
cause::type acpi_init();

namespace acpi {

/// @brief MADT のエントリを列挙する。
//
/// @pre AcpiInitializeTables() is finished.
class madt_iterator
{
public:
	madt_iterator();
	const ACPI_SUBTABLE_HEADER* get_next_header();
	const ACPI_SUBTABLE_HEADER* get_next_header(u8 type);

private:
	const ACPI_TABLE_MADT* madt_table;
	const u8*              next_entry;
};

/// @pre AcpiInitializeTables() is finished.
template<class DATA, u8 TYPEID>
class madt_typed_iterator : public madt_iterator
{
public:
	const DATA* get_next_entry() {
		return reinterpret_cast<const DATA*>(get_next_header(TYPEID));
	}
};

typedef madt_typed_iterator<ACPI_MADT_LOCAL_APIC, ACPI_MADT_TYPE_LOCAL_APIC>
    madt_local_apic_iterator;
typedef madt_typed_iterator<ACPI_MADT_IO_APIC, ACPI_MADT_TYPE_IO_APIC>
    madt_io_apic_iterator;

}  // namespace acpi


#endif  // CORE_INCLUDE_ACPI_CTL_HH_

