/// @file  core/acpi_ctl.hh
//
// (C) 2012-2015 KATO Takeshi
//

#ifndef CORE_ACPI_CTL_HH_
#define CORE_ACPI_CTL_HH_

extern "C" {
#include <acpi.h>
}  // extern "C"

#include <arch/pagetable.hh>


namespace acpi {

cause::t table_init(uptr buffer_padr, arch::page::TYPE buffer_type);
cause::t init();
///*
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
//*/

/// @brief Enumerate ACPI_SUBTABLE_HEADER entries.
class subtable_enumerator
{
public:
	template <class TBL> subtable_enumerator(TBL* acpi_tbl) :
		subtbl_end(
		    reinterpret_cast<u8*>(acpi_tbl) + acpi_tbl->Header.Length),
		subtbl_entry(
		    reinterpret_cast<u8*>(acpi_tbl) + sizeof *acpi_tbl)
	{
	}

	ACPI_SUBTABLE_HEADER* next()
	{
		if (subtbl_entry >= subtbl_end)
			return nullptr;

		auto r = subtbl_hdr();
		subtbl_entry += r->Length;

		return r;
	}

	ACPI_SUBTABLE_HEADER* next(u8 type)
	{
		while (subtbl_entry < subtbl_end) {
			auto r = subtbl_hdr();
			subtbl_entry += r->Length;

			if (r->Type == type)
				return r;
		}
		return nullptr;
	}

private:
	ACPI_SUBTABLE_HEADER* subtbl_hdr() {
		return reinterpret_cast<ACPI_SUBTABLE_HEADER*>(subtbl_entry);
	}

private:
	u8* subtbl_end;
	u8* subtbl_entry;
};

}  // namespace acpi


#endif  // include guard

