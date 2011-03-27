/// @file  hpet_timer.cc
/// @brief HPET timer.
/// マルチCPU環境で眠っているCPUを起こすためのタイマ。
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "misc.hh"
#include "mpspec.hh"


namespace {

/// checksum
u8 sum8(const void* ptr, u32 length)
{
	const u8* p = reinterpret_cast<const u8*>(ptr);
	u8 sum = 0;
	for (u32 i = 0; i < length; ++i)
		sum += p[i];

	return sum;
}

/// signature
inline u32 sig32(u32 c1, u32 c2, u32 c3, u32 c4)
{
	return c1 | c2 << 8 | c3 << 16 | c4 << 24;
}

/// Root System Description Pointer Structure
struct rsdp
{
	char signature[8];
	u8   checksum;
	char oemid[6];
	u8   revision;
	u32  rsdt_adr;
	u32  length;  // maybe sizeof xsdt
	u64  xsdt_adr;
	u8   ext_checksum;
	u8   reserved[3];

	bool test() const;
};

/// test signature and checksum
bool rsdp::test() const
{
	static const char sig[8] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };

	for (int i = 0; i < 8; ++i) {
		if (signature[i] != sig[i])
			return false;
	}

	if (sum8(this, 20 /* ACPI 1.0 spec */) != 0)
		return false;

	return sum8(this, sizeof *this) == 0;
}

const rsdp* scan_rsdp(u32 base, u32 length)
{
	const u8* ptr = reinterpret_cast<const u8*>(base);

	// RSDP must be aligned to 16bytes.

	for (u32 i = 0; i < length; i += 16) {
		const rsdp* p = reinterpret_cast<const rsdp*>(ptr + i);
		if (p->test())
			return p;
	}

	return 0;
}

/// @brief  search RSDP(Root System Description Pointer)
const rsdp* search_rsdp()
{
	// search from EBDA(Extended BIOS Data Area)
	const u16 ebda = *reinterpret_cast<u16*>(0x40e);
	const rsdp* rsdp = scan_rsdp(ebda, 1024);

	if (rsdp == 0)
		// search from BIOS
		rsdp = scan_rsdp(0xe0000, 0x1ffff);

	return rsdp;
}

/// RSDT(Root System Description Table) header
struct rsdt_header
{
	char signature[4];
	u32  length;  // size of header and table
	u8   revision;
	u8   checksum;
	char oemid[6];
	char oem_table_id[8];
	u32  oem_revision;
	char creator_id[4];
	u32  creator_revision;

	u32 get_entry_count() const {
		return (length - sizeof *this) / sizeof (u32);
	}
	const u32* get_entry_table() const {
		return reinterpret_cast<const u32*>(this + 1);
	}

	bool test() const;
};

/// test signature.
bool rsdt_header::test() const
{
	static const char sig[4] = { 'R', 'S', 'D', 'T' };

	for (int i = 0; i < 4; ++i) {
		if (signature[i] != sig[i])
			break;
	}

	return sum8(this, length) == 0;
}

/// System Description Table Header
struct DESCRIPTION_HEADER
{
	union {
		char signature[4];
		u32 signature32;
	};
	u32  length;
	u8   revision;
	u8   checksum;
	char oemid[6];
	char oem_table_id[8];
	u32  oem_revision;
	char creator_id[4];
	u32  creator_revision;

	/// test checksum.
	bool test() const {
		return sum8(this, length) == 0;
	}
};


/// MADT (Multiple APIC Description Table)
struct madt
{
	DESCRIPTION_HEADER header;
	u32  lapic_adr;
	u32  flags;

	// APIC structure
	u32 get_structure_length() const {
		return header.length - sizeof *this;
	}
	const u8* get_structure_table() const {
		return reinterpret_cast<const u8*>(this + 1);
	}
};

const DESCRIPTION_HEADER* search_desc_by_sig(u32 sig, const rsdt_header* rsdth)
{
	const u32* table = rsdth->get_entry_table();
	const u32 n = rsdth->get_entry_count();

	for (u32 i = 0; i < n; ++i) {
		DESCRIPTION_HEADER* dh =
		    reinterpret_cast<DESCRIPTION_HEADER*>(table[i]);
		if (dh->signature32 == sig)
			return dh;
	}

	return 0;
}

const u8* search_ioapic_struct(const u8* table, u32 length)
{
	for (u32 i = 0; i < length; ++i) {
		const u8 type = table[i];
		const u8 len = table[i + 1];

		if (type == 1)
			return &table[i];

		if (len == 0)
			break;
		i += len;
	}

	return 0;
}

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

class ioapic_control
{
	struct memmapped_regs {
		u32 volatile ioregsel;
		u32          unused[3];
		u32 volatile iowin;
	};
	memmapped_regs* const regs;

public:
	ioapic_control(u32 padr) 
	    : regs(reinterpret_cast<memmapped_regs*>(
	           arch::pmem::direct_map(padr)))
	{}
	u32 read(u32 sel) {
		regs->ioregsel = sel;
		return regs->iowin;
	}
	void write(u32 sel, u32 data) {
		regs->ioregsel = sel;
		regs->iowin = data;
	}
	void mask(u32 index) {
		write(0x10 + index * 2, 0x00010000);
	}
	void unmask(u32 index, u8 cpuid, u8 vec) {
		// edge trigger, physical destination, fixd delivery
		write(0x10 + index * 2 + 1, static_cast<u32>(cpuid) << 24);
		write(0x10 + index * 2, vec);
	}
};


/// I/O APIC が HPET の割り込みを受け入れるよう設定する。
bool ioapic_setup(const rsdt_header* rsdth)
{
	mpspec::const_mpfps* mpfps = search_mpfps();
	if (mpfps == 0)
		return false;

	const mpspec::ioapic_entry* ioapic_etr = search_ioapic(mpfps);
	if (ioapic_etr == 0)
		return false;

	rsdth = rsdth;
/*
	const madt* desc = reinterpret_cast<const madt*>(
	    search_desc_by_sig(sig32('A', 'P', 'I', 'C'), rsdth));
	if (!desc)
		return false;

	const u8* ioapic_desc = search_ioapic_struct(
	    desc->get_structure_table(),
	    desc->get_structure_length());
	if (!ioapic_desc)
		return false;
*/

	ioapic_control ioapic(ioapic_etr->ioapic_map_padr);
	log()("ioapic id = ").u(ioapic.read(0), 16)();
	log()("ioapic ver = ").u(ioapic.read(1), 16)();
	log()("red 10 = ").u(ioapic.read(0x10), 16)();
	log()("red 11 = ").u(ioapic.read(0x11), 16)();
	log()("red 12 = ").u(ioapic.read(0x12), 16)();
	log()("red 13 = ").u(ioapic.read(0x13), 16)();
	log()("red 14 = ").u(ioapic.read(0x14), 16)();
	log()("red 15 = ").u(ioapic.read(0x15), 16)();


	return true;
}

}  // namespace


bool hpet_init()
{
	const rsdp* rsdp = search_rsdp();

	const rsdt_header* rsdth =
	    reinterpret_cast<const rsdt_header*>(rsdp->rsdt_adr);

	if (!rsdth->test())
		log()("RSDT test failed.")();

	const u32* e = rsdth->get_entry_table();
	for (u32 i = 0; i < rsdth->get_entry_count(); ++i) {
		log()("entry[").u(i)("] = ").u(e[i], 16)();
		DESCRIPTION_HEADER* dh = (DESCRIPTION_HEADER*)e[i];
		if (!dh->test())
			log()("DESCRIPTION_HEADER[").u(i)("] test failed.")();
		log()("dh[").u(i)("].signature = ")
		    (dh->signature[0])
		    (dh->signature[1])
		    (dh->signature[2])
		    (dh->signature[3])();
	}

	if (!ioapic_setup(rsdth))
		return false;

	return true;
}

void hpet_uninit()
{
}

