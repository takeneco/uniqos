/// @file  hpet_timer.cc
/// @brief HPET timer.
/// マルチCPU環境で眠っているCPUを起こすためのタイマ。
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "misc.hh"
#include "mpspec.hh"
#include "native_ops.hh"


namespace {

enum timer_index
{
	TIMER_0 = 0,
	TIMER_1 = 1,
};

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

// ここから ACPI
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// ここまで ACPI

struct hpet_desc
{
	DESCRIPTION_HEADER header;
	u32  eventtimer_block_id;
	u32  base_adr[3];  // 12bytes
	u8   hpet_number;
	u8   minimum_clock_tick_in_periodic[2];
	u8   attributes;
};

struct hpet_register
{
	u32          caps_l32;
	u32          caps_h32;
	u64          reserved1;
	u64 volatile configs;
	u64          reserved2;
	u64 volatile intr_status;
	u64          reserved3[25];
	u64 volatile counter;
	u64          reserved4;

	struct timer_regs {
		u32 volatile config_l32;
		u32 volatile config_h32;
		u64 volatile comparator;
		u64 volatile fsb_intr;
		u64          reserved;
	};
	timer_regs timer[32];

	void enable_legrep() { configs |= 0x3; }
	void disable() { configs &= ~0x3; }

	u64 usecs_to_count(u64 usecs) const {
		return usecs * 1000000000 / caps_h32;
	}

	/// @brief  enable nonperiodic timer.
	/// - not FSB delivery.
	/// - 64bit timer
	/// - edge trigger.
	/// @param  i  index of timer.
	/// @param  intr_route  I/O APIC input pin number.
	///   range : 0-31
	///   ignored in LegacyReplacement mode.
	void set_nonperiodic_timer(timer_index i, u32 intr_route=0)
	{
		timer[i].config_l32 =
		    (timer[i].config_l32 & 0xffff8030) |
		    0x00000004 |
		    (intr_route << 9);
	}

	/// @brief  enable periodic timer
	/// 周期タイマを使うときはこの関数だけでは設定できない。
	/// 初期化手順から合わせる必要がある。
	void set_periodic_timer(
	    timer_index i, u64 interval_usecs, u32 intr_route=0)
	{
		timer[i].config_l32 =
		    (timer[i].config_l32 & 0xffff8030) |
		    0x0000004c |
		    (intr_route << 9);
		timer[i].comparator =
		    interval_usecs * 1000000000 / caps_h32;
	}

	void unset_timer(timer_index i) {
		timer[i].config_l32 = timer[i].config_l32 & 0xffff8030;
		timer[i].comparator = U64CAST(0xffffffffffffffff);
	}

	/// set time on nonperiodicmode
	/// @param  usecs  specify micro secs.
	u64 set_oneshot_time(timer_index i, u64 usecs) {
		const u64 time = counter + usecs_to_count(usecs);
		timer[i].comparator = time;
		return time;
	}
};

hpet_register* hpet_detect()
{
	const rsdp* rsdp = search_rsdp();
	if (!rsdp) {
		log()("RSDP search failed.")();
		return 0;
	}

	const rsdt_header* rsdth =
	    reinterpret_cast<const rsdt_header*>(rsdp->rsdt_adr);
	if (!rsdth->test()) {
		log()("RSDT test failed.")();
		return 0;
	}

	const u32* e = rsdth->get_entry_table();
	hpet_desc* hpetdesc = 0;
	for (u32 i = 0; i < rsdth->get_entry_count(); ++i) {
		DESCRIPTION_HEADER* dh =
		    reinterpret_cast<DESCRIPTION_HEADER*>(e[i]);
		if (!dh->test()) {
			log()("DESCRIPTION_HEADER ")(dh)(" test failed.")();
			return 0;
		}
		if (dh->signature32 == sig32('H', 'P', 'E', 'T')) {
			hpetdesc = reinterpret_cast<hpet_desc*>(dh);
			break;
		}
	}
	if (!hpetdesc) {
		log()("HPET not found.")();
		return 0;
	}

	const u64 hpet_regadr =
	    static_cast<u64>(hpetdesc->base_adr[1]) |
	    static_cast<u64>(hpetdesc->base_adr[2]) << 32;

	return reinterpret_cast<hpet_register*>(
	    arch::pmem::direct_map(hpet_regadr));
}

hpet_register* hpet_regs;

}  // namespace


bool hpet_init()
{
	hpet_register* hpetregs = hpet_detect();
	hpet_regs = hpetregs;

	if (hpetregs == 0)
		return false;

	// setup periodic timer
	hpetregs->disable();
	hpetregs->counter = 0;
	hpetregs->set_periodic_timer(TIMER_0, 1000000 /* 1sec */);
	hpetregs->enable_legrep();

	return true;
}

/// @param usecs  specify mili secs.
void timer_sleep(u32 msecs)
{
	native::sti();

	hpet_regs->set_nonperiodic_timer(TIMER_1, 8);
	const u64 limit = hpet_regs->set_oneshot_time(TIMER_1, 1000 * msecs);

	do {
		native::hlt();
	} while (hpet_regs->counter < limit);

	hpet_regs->unset_timer(TIMER_1);

	native::cli();
}

extern "C" void on_interrupt()
{
	volatile u32* eoi = reinterpret_cast<u32*>(arch::pmem::direct_map(0xfee000b0));
	*eoi = 0;

	log()("intr &eoi = ")(&eoi)();
}
