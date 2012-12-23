/// @file   acpi_ctl.cc
//
// (C) 2012 KATO Takeshi
//

#include <acpi_ctl.hh>

#include <log.hh>

namespace {
void acpi_walk();
}  // namespace

/// @brief AcpiInitializeTables() を呼び出す。
/// @param[in] size   size of buffer.
/// @param[in] buffer InitialTableArray として使うバッファ。
///                   1ページを渡しておくと後で開放しやすい。
//
/// この関数の実行後は、OS の準備ができていなくても CPU の数を
/// 数えたりすることができるようになる。
cause::type acpi_table_init(uptr size, void* buffer)
{
	AcpiDbgLevel = 0;
	AcpiDbgLayer = 0;

	ACPI_STATUS as = AcpiInitializeTables(
	    static_cast<ACPI_TABLE_DESC*>(buffer),
	    size / sizeof (ACPI_TABLE_DESC),
	    TRUE);
	if (ACPI_FAILURE(as))
		return cause::FAIL;

///////////////////
/*
	log()("acpi_buffer:")(buffer)();

	ACPI_TABLE_HEADER* ath;
	as = AcpiGetTable("APIC", 0, &ath);
	if (ACPI_FAILURE(as))
		return cause::FAIL;

	log()("ACPI Table:\n")
		(" Signature:")
		.c(ath->Signature[0])
		.c(ath->Signature[1])
		.c(ath->Signature[2])
		.c(ath->Signature[3])()
		(" Length:").u(ath->Length)()
		(" Revision:").u(ath->Revision)()
		(" Checksum:").x(ath->Checksum)()
		(" OemId:")
		.c(ath->OemId[0])
		.c(ath->OemId[1])
		.c(ath->OemId[2])
		.c(ath->OemId[3])
		.c(ath->OemId[4])
		.c(ath->OemId[5])()
		(" OemTableId:")
		.c(ath->OemTableId[0])
		.c(ath->OemTableId[1])
		.c(ath->OemTableId[2])
		.c(ath->OemTableId[3])
		.c(ath->OemTableId[4])
		.c(ath->OemTableId[5])
		.c(ath->OemTableId[6])
		.c(ath->OemTableId[7])()
		(" OemRevision:").x(ath->OemRevision)()
		(" AslCompilerId:")
		.c(ath->AslCompilerId[0])
		.c(ath->AslCompilerId[1])
		.c(ath->AslCompilerId[2])
		.c(ath->AslCompilerId[3])()
		(" AslCompilerRevision:").u(ath->AslCompilerRevision)();
*/
///////////////////
	return cause::OK;
}

cause::type acpi_init()
{
	ACPI_STATUS as = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(as)) {
		return cause::FAIL;
	}

	//as = AcpiInitializeTables(0, 16, FALSE);
	as = AcpiReallocateRootTable();
	if (ACPI_FAILURE(as)) {
		log()(SRCPOS)();
		return cause::FAIL;
	}

	as = AcpiLoadTables();
	if (ACPI_FAILURE(as)) {
		log()(SRCPOS)();
		return cause::FAIL;
	}

	as = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as)) {
		log()(SRCPOS)();
		return cause::FAIL;
	}

	as = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as)) {
		log()(SRCPOS)();
		return cause::FAIL;
	}

	//acpi_walk();

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

