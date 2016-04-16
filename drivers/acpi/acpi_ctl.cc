/// @file   acpi_ctl.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
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

#include <core/acpi_ctl.hh>

#include <core/log.hh>


namespace {

uptr             static_table_padr;
arch::page::TYPE static_table_type;

void acpi_walk();
}  // namespace

namespace acpi {

/// @brief AcpiInitializeTables() を呼び出す。
/// @pre Physical Address Map(PAM) が使用可能なこと。
/// @param[in] buffer_padr  Physical buffer address as InitialTableArray.
/// @param[in] buffer_type  Page type of buffer_padr.
//
/// この関数の実行後は、OS の準備ができていなくても CPU の数を
/// 数えたりすることができるようになる。
cause::t table_init(uptr buffer_padr, arch::page::TYPE buffer_type)
{
	//AcpiDbgLevel = 0;
	//AcpiDbgLayer = 0;

	uptr bytes = arch::page::size_of_type(buffer_type);
	void* buffer = arch::map_phys_adr(buffer_padr, bytes);

	ACPI_STATUS as = AcpiInitializeTables(
	    static_cast<ACPI_TABLE_DESC*>(buffer),
	    bytes / sizeof (ACPI_TABLE_DESC),
	    TRUE);
	if (ACPI_FAILURE(as))
		return cause::FAIL;

	static_table_padr = buffer_padr;
	static_table_type = buffer_type;

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

	struct MADT_ENT {
		u8 Type;
		u8 Length;
	};
	int len = ath->Length - sizeof (*ath);
	u8* tbl = (u8*)ath + ath->Length;
	while (len > 0) {
		MADT_ENT* e = (MADT_ENT*)tbl;
		len -= e->Length;
		tbl += e->Length;
		log()("T:").u(e->Type)(" L:").u(e->Length)();
	}
*/
///////////////////
	return cause::OK;
}

cause::t init()
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
///*
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
//*/
}  // namespace acpi

ACPI_STATUS disp_obj(
    ACPI_HANDLE h, u32 /*level*/, void* /*Context*/, void** ReturnValue)
{
	ACPI_STATUS       Status;
	ACPI_DEVICE_INFO* Info;
	ACPI_BUFFER       Path;
	char              Buffer[512];

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

	if (Info->Type == ACPI_TYPE_DEVICE) {
		ACPI_BUFFER Res;
		Res.Length = sizeof Buffer;
		Res.Pointer = Buffer;
		Status = AcpiGetCurrentResources(h, &Res);
		if (ACPI_SUCCESS(Status)) {
			ACPI_RESOURCE* ar = (ACPI_RESOURCE*)Res.Pointer;
			log()("ResType: ").u(ar->Type)();
			if (ar->Type == ACPI_RESOURCE_TYPE_DMA) {
				log()("DmaType   :").u(ar->Data.Dma.Type)()
				     ("BusMaster :").u(ar->Data.Dma.BusMaster)()
				     ("Transfer  :").u(ar->Data.Dma.Transfer)()
				     ("ChCnt     :").u(ar->Data.Dma.ChannelCount)()
				     ("Ch[0]     :").u(ar->Data.Dma.Channels[0])();
			}
		}
		else {
			log()(SRCPOS)("(): AcpiGetCurrentResources() failed.Status=0x").x(Status)();
		}
	}

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
	for (int i = 0; ; ++i) {
		ACPI_TABLE_MADT* madt_table;
		char acpi_sig_madt[] = ACPI_SIG_MADT;
		r = AcpiGetTable(acpi_sig_madt, i,
		    reinterpret_cast<ACPI_TABLE_HEADER**>(&madt_table));
		if (ACPI_FAILURE(r)) {
			log()("table:fail(").u(r)(")")();
			break;
		}
		log()("i=").u(i)(" madt_table=")(madt_table)();
		log()("i=").u(i)(" Address=").x(madt_table->Address)();
	}

	log()("\nAcpiGetTable: HPET")();
	ACPI_TABLE_HPET* hpet_table;
	char acpi_sig_hpet[] = ACPI_SIG_HPET;
	r = AcpiGetTable(acpi_sig_hpet, 0, reinterpret_cast<ACPI_TABLE_HEADER**>(&hpet_table));
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

void acpi_log()
{
	ACPI_TABLE_MADT* ath;
	char sig[] = ACPI_SIG_MADT;
	ACPI_STATUS as = AcpiGetTable(sig, 0, (ACPI_TABLE_HEADER**)&ath);
	if (ACPI_FAILURE(as)) {
		log()("ACPI Fail")();
		return ;
	}

	log()("ACPI Table:\n")
		(" Signature:")
		.c(ath->Header.Signature[0])
		.c(ath->Header.Signature[1])
		.c(ath->Header.Signature[2])
		.c(ath->Header.Signature[3])()
		(" Length:").u(ath->Header.Length)()
		(" Revision:").u(ath->Header.Revision)()
		(" Checksum:").x(ath->Header.Checksum)()
		(" OemId:")
		.c(ath->Header.OemId[0])
		.c(ath->Header.OemId[1])
		.c(ath->Header.OemId[2])
		.c(ath->Header.OemId[3])
		.c(ath->Header.OemId[4])
		.c(ath->Header.OemId[5])()
		(" OemTableId:")
		.c(ath->Header.OemTableId[0])
		.c(ath->Header.OemTableId[1])
		.c(ath->Header.OemTableId[2])
		.c(ath->Header.OemTableId[3])
		.c(ath->Header.OemTableId[4])
		.c(ath->Header.OemTableId[5])
		.c(ath->Header.OemTableId[6])
		.c(ath->Header.OemTableId[7])()
		(" OemRevision:").x(ath->Header.OemRevision)()
		(" AslCompilerId:")
		.c(ath->Header.AslCompilerId[0])
		.c(ath->Header.AslCompilerId[1])
		.c(ath->Header.AslCompilerId[2])
		.c(ath->Header.AslCompilerId[3])()
		(" AslCompilerRevision:").u(ath->Header.AslCompilerRevision)();

	struct MADT_ENT {
		u8 Type;
		u8 Length;
	};
	int len = ath->Header.Length - sizeof *ath;
	u8* tbl = (u8*)ath + sizeof *ath;
	while (len > 0) {
		MADT_ENT* e = (MADT_ENT*)tbl;
		if (e->Length == 0)
			break;
		len -= e->Length;
		tbl += e->Length;
		log()("T:").u(e->Type)(" L:").u(e->Length)();
	}

	ACPI_TABLE_SRAT* rsat;
	char sig_rsat[] = ACPI_SIG_SRAT;
	as = AcpiGetTable(sig_rsat, 0, (ACPI_TABLE_HEADER**)&rsat);
	if (ACPI_FAILURE(as)) {
		log()("ACPI Fail")();
		return ;
	}
	log()("ACPI Table:\n")
		(" Signature:")
		.c(rsat->Header.Signature[0])
		.c(rsat->Header.Signature[1])
		.c(rsat->Header.Signature[2])
		.c(rsat->Header.Signature[3])()
		(" Length:").u(rsat->Header.Length)()
		(" Revision:").u(rsat->Header.Revision)()
		(" Checksum:").x(rsat->Header.Checksum)()
		(" OemId:")
		.c(rsat->Header.OemId[0])
		.c(rsat->Header.OemId[1])
		.c(rsat->Header.OemId[2])
		.c(rsat->Header.OemId[3])
		.c(rsat->Header.OemId[4])
		.c(rsat->Header.OemId[5])()
		(" OemTableId:")
		.c(rsat->Header.OemTableId[0])
		.c(rsat->Header.OemTableId[1])
		.c(rsat->Header.OemTableId[2])
		.c(rsat->Header.OemTableId[3])
		.c(rsat->Header.OemTableId[4])
		.c(rsat->Header.OemTableId[5])
		.c(rsat->Header.OemTableId[6])
		.c(rsat->Header.OemTableId[7])()
		(" OemRevision:").x(rsat->Header.OemRevision)()
		(" AslCompilerId:")
		.c(rsat->Header.AslCompilerId[0])
		.c(rsat->Header.AslCompilerId[1])
		.c(rsat->Header.AslCompilerId[2])
		.c(rsat->Header.AslCompilerId[3])()
		(" AslCompilerRevision:").u(rsat->Header.AslCompilerRevision)();

	len = rsat->Header.Length - sizeof *rsat;
	tbl = (u8*)rsat + sizeof *rsat;
	while (len > 0) {
		ACPI_SUBTABLE_HEADER* e = (ACPI_SUBTABLE_HEADER*)tbl;
		if (e->Length == 0)
			break;
		len -= e->Length;
		tbl += e->Length;
		log()("T:").u(e->Type)(" L:").u(e->Length)();
		switch (e->Type) {
		case ACPI_SRAT_TYPE_CPU_AFFINITY: {
			ACPI_SRAT_CPU_AFFINITY* _e = (ACPI_SRAT_CPU_AFFINITY*)e;
			log()
			("ProximityDomainLo: ").u(_e->ProximityDomainLo)()
			("ApicId: ").u(_e->ApicId)()
			("Flags: ").x(_e->Flags)()
			("LocalSapicEid: ").u(_e->LocalSapicEid)()
			("ProximityDomainHi: ")
			.u(_e->ProximityDomainHi[0])(' ')
			.u(_e->ProximityDomainHi[1])(' ')
			.u(_e->ProximityDomainHi[2])();
			//("Reserved :").u(_e->Reserved)();
		}
			break;
		case ACPI_SRAT_TYPE_MEMORY_AFFINITY: {
			ACPI_SRAT_MEM_AFFINITY* _e = (ACPI_SRAT_MEM_AFFINITY*)e;
			log()
			("ProximityDomain: ").u(_e->ProximityDomain)()
			("Reserved: ").u(_e->Reserved)()
			("BaseAddress: ").x(_e->BaseAddress)()
			("Length: ").x(_e->Length)()
			("Reserved1: ").u(_e->Reserved1)()
			("Flags: ").x(_e->Flags)()
			("Reserved2: ").u(_e->Reserved2)();
		}
			break;
		case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY:
			break;
		}
	}


	return;
}

