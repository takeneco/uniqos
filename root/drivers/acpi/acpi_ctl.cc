/// @file   acpi_ctl.cc
//
// (C) 2012 KATO Takeshi
//

#include <acpi_ctl.hh>

#include <log.hh>

namespace {
void acpi_walk();
}  // namespace

cause::type acpi_init()
{
	ACPI_STATUS as = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	as = AcpiInitializeTables(0, 16, FALSE);
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	as = AcpiLoadTables();
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	as = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	as = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	acpi_walk();

	return cause::OK;
}

namespace acpi {

madt_iterator::madt_iterator()
{
	ACPI_TABLE_HEADER* table_header;
	char sig[] = ACPI_SIG_MADT;
	ACPI_STATUS r = AcpiGetTable(sig, 0, &table_header);
	if (ACPI_FAILURE(r)) {
		madt_table = 0;
		return;
	}

	madt_table = reinterpret_cast<const ACPI_TABLE_MADT*>(table_header);

	next_entry = reinterpret_cast<const u8*>(madt_table + 1);
}

const ACPI_SUBTABLE_HEADER* madt_iterator::get_next_header()
{
	if (!madt_table)
		return 0;

	const ACPI_SUBTABLE_HEADER* ret =
	    reinterpret_cast<const ACPI_SUBTABLE_HEADER*>(next_entry);

	const uptr _ret = reinterpret_cast<uptr>(ret);
	const uptr end = reinterpret_cast<uptr>(madt_table) +
	                 madt_table->Header.Length;
	if (_ret >= end)
		return 0;

	next_entry = next_entry + ret->Length;

	return ret;
}

const ACPI_SUBTABLE_HEADER* madt_iterator::get_next_header(u8 type)
{
	for (;;) {
		const ACPI_SUBTABLE_HEADER* h = get_next_header();
		if (!h)
			return h;

		if (h->Type == type)
			return h;
	}
}

}  // namespace acpi

ACPI_STATUS disp_obj(
    ACPI_HANDLE h, u32 level, void* Context, void** ReturnValue)
{
	ACPI_STATUS       Status;
	ACPI_DEVICE_INFO* Info;
	ACPI_BUFFER       Path;
	char              Buffer[256];

	Path.Length = sizeof Buffer;
	Path.Pointer = Buffer;

	Status = AcpiGetName(h, ACPI_FULL_PATHNAME, &Path);
	if (ACPI_SUCCESS(Status))
		log()("path: ").str(static_cast<const char*>(Path.Pointer))();
	else
		log()("path:fail")();

	Status = AcpiGetObjectInfo(h, &Info);
	if (ACPI_SUCCESS(Status))
		log()("  HID: ")(Info->HardwareId.String)
		     (", ADR: ").x(Info->Address)
		     (", Status: ").x(Info->CurrentStatus)();
	else
		log()("  info:fail")();

	*ReturnValue = 0;

	return AE_OK;
}
namespace {
void acpi_walk()
{
	ACPI_STATUS r;
	void* UserContext = 0;
	void* ReturnValue;
	AcpiWalkNamespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
	    100, disp_obj, 0, UserContext,
	    &ReturnValue);

	log()("\nAcpiGetTable: APIC")();
	ACPI_TABLE_MADT* madt_table;
	r = AcpiGetTable(ACPI_SIG_MADT, 0, reinterpret_cast<ACPI_TABLE_HEADER**>(&madt_table));
	if (ACPI_FAILURE(r))
		log()("table:fail(").u(r)(")")();
	log()("madt_table=")(madt_table)();
	log()("Address=").x(madt_table->Address)();


	log()("\nAcpiGetTable: HPET")();
	ACPI_TABLE_HPET* hpet_table;
	r = AcpiGetTable(ACPI_SIG_HPET, 0, reinterpret_cast<ACPI_TABLE_HEADER**>(&hpet_table));
	if (ACPI_FAILURE(r))
		log()("table:fail(").u(r)(")")();
	log()("hpet_table=")(hpet_table)();
	log()("Address.SpaceId=").x(hpet_table->Address.SpaceId)();
	log()("Address.BitWidth=").x(hpet_table->Address.BitWidth)();
	log()("Address.BitOffset=").x(hpet_table->Address.BitOffset)();
	log()("Address.AccessWidth=").x(hpet_table->Address.AccessWidth)();
	log()("Address.Address=").x(hpet_table->Address.Address)();

	log()("\nLocal APIC:")();
	acpi::madt_local_apic_iterator lapic_itr;
	for (;;) {
		const ACPI_MADT_LOCAL_APIC* la = lapic_itr.get_next_entry();
		if (!la)
			break;

		log()("ProcessorId=").u(la->ProcessorId)()
		     ("Id=").u(la->Id)()
		     ("LapicFlags=").x(la->LapicFlags)();
	}
}
}  // namespace

