/// @file   cpuinit.cc
/// @brief  Initialize GDT/IDT.
//
// (C) 2010 KATO Takeshi
//

#include "kerninit.hh"

#include "arch.hh"
#include "btypes.hh"
#include "desctable.hh"
#include "native_ops.hh"
#include "setupdata.hh"

#include "log.hh"


namespace {

/// Global Descriptor Table Entry
class gdte
{
protected:
	typedef u64 type;
	type e;

public:
	enum {
		XR  = U64CAST(0xa) << 40, ///< Exec and Read.
		RW  = U64CAST(0x2) << 40, ///< Read and Write

		/// System segment if clear,
		/// Data or Code segment if set.
		S   = U64CAST(1) << 44,

		P   = U64CAST(1) << 47,  ///< Descriptor exist if set.
		AVL = U64CAST(1) << 52,  ///< Software useable.
		L   = U64CAST(1) << 53,  ///< Long mode if set.
		D   = U64CAST(1) << 54,  ///< If long mode, must be clear.
		G   = U64CAST(1) << 55,  ///< Limit scale 4096 times if set.
	};

	gdte() {}
	void set_null() {
		e = 0;
	}
	void set(type base, type limit, type dpl, type flags) {
		e = (base  & 0x00ffffff) << 16 |
		    (base  & 0xff000000) << 32 |
		    (limit & 0x0000ffff)       |
		    (limit & 0x000f0000) << 32 |
		    (dpl & 3) << 45 |
		    flags;
	}
	type get_raw() const { return e; }
};

class code_seg_desc : gdte
{
public:
	code_seg_desc() : gdte() {}

	/// flags には AVL を指定できる。
	void set(type dpl, type flags=0) {
		// base and limit is disabled in long mode.
		gdte::set(0, 0, dpl, XR | S | P | L | flags);
	}
};

class data_seg_desc : gdte
{
public:
	data_seg_desc() : gdte() {}

	/// flags には AVL を指定できる。
	void set(type dpl, type flags=0) {
		// base and limit is disabled in long mode.
		gdte::set(0, 0, dpl, RW | S | P | L | flags);
	}
};

class global_desc_table
{
	static global_desc_table* nulltable() { return 0; }
	static u16 offset_cast(void* p) {
		return static_cast<u16>(reinterpret_cast<uptr>(p));
	}

public:
	gdte          null_entry;
	code_seg_desc kern_code;
	data_seg_desc kern_data;
	code_seg_desc user_code;
	data_seg_desc user_data;

	static u16 kern_code_offset() {
		return offset_cast(&nulltable()->kern_code);
	}
	static u16 kern_data_offset() {
		return offset_cast(&nulltable()->kern_data);
	}
};

global_desc_table cpu0_gdt;


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

/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mp_floating_pointer_structure::test() const
{
	// signature test
	if (signature != le32_to_cpu('_', 'M', 'P', '_')) {
		return false;
	}

	// checksum test
	const u8* p = reinterpret_cast<const u8*>(this);
	u8 sum = 0;
	for (unsigned i = 0; i < sizeof *this; i++)
		sum += p[i];

	return sum == 0;
}


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

/// valid test.
/// @retval  true  valid.
/// @retval  false invalid.
bool mp_configuration_table_header::test() const
{
	// signature test
	if (signature != le32_to_cpu('P', 'C', 'M', 'P')) {
		return false;
	}

	// checksum test
	const u8* p = reinterpret_cast<const u8*>(this);
	u8 sum = 0;
	for (u16 i = 0; i < base_table_length; i++)
		sum += p[i];

	return sum == 0;
}


/// Processor entry
struct processor_entry
{
	u8  entry_type;          ///< =0
	u8  local_apic_id;
	u8  local_apic_version;
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
	u8  entry_type;   ///< =1
	u8  bus_id;
	u8  bus_type[6];

	const u8* next_entry() const {
	    return reinterpret_cast<const u8*>(this + 1);
	}
};


/// I/O APIC entry
struct io_apic_entry
{
	u8  entry_type;        ///< =2
	u8  io_apic_id;
	u8  io_apic_version;
	u8  io_apic_flags;
	u32 io_apic_map_padr;

	const u8* next_entry() const {
	    return reinterpret_cast<const u8*>(this + 1);
	}
};


/// I/O interrupt assignment entry
struct io_int_entry
{
	u8  entry_type;          ///< =3
	u8  int_type;
	u16 io_int_flags;
	u8  source_bus_id;
	u8  source_bus_irq;
	u8  dest_io_apic_id;
	u8  dest_io_apic_intin;

	const u8* next_entry() const {
	    return reinterpret_cast<const u8*>(this + 1);
	}
};


/// Local interrupt assignment entry
struct local_int_entry
{
	u8  entry_type;             ///< =4
	u8  int_type;
	u16 local_int_flags;
	u8  source_bus_id;
	u8  source_bus_irq;
	u8  dest_local_apic_id;
	u8  dest_local_apic_intin;

	const u8* next_entry() const {
	    return reinterpret_cast<const u8*>(this + 1);
	}
};


/// CPU
class processor_unit
{
public:
	u8  id;
};
enum { PROCESSOR_COUNT_MAX = 2 };
processor_unit processor[PROCESSOR_COUNT_MAX];
u32 cpu_count;
u32 tmp[5];


/// @return  returns ptr to mpfps.
/// @return  if floating ptr not found returns 0.
mp_floating_pointer_structure* scan_mpfps(u32 start_padr, u32 length)
{
	uptr base = arch::PHYSICAL_MEMMAP_BASEADR + start_padr;
	const u32 end = length - sizeof (mp_floating_pointer_structure);
	for (u32 i = 0; i <= end; i += 4) {
		mp_floating_pointer_structure* p =
		    reinterpret_cast<mp_floating_pointer_structure*>(base + i);
		if (p->test())
			return p;
	}

	return 0;
}

/// search EBDA(Extended BIOS Data Area)
/// @return  returns ptr to mpfps.
/// @return  if no mpfps returns 0.
mp_floating_pointer_structure* search_mpfps()
{
	uptr ebda = *reinterpret_cast<u16*>(
	    arch::PHYSICAL_MEMMAP_BASEADR + 0x40e) << 4;
	mp_floating_pointer_structure* fp = scan_mpfps(ebda, 0x400);
	if (fp != 0)
		return fp;

	fp = scan_mpfps(0xf0000, 0x10000);
	if (fp != 0)
		return fp;

	fp = scan_mpfps(0x9fc00, 0x400);
	if (fp != 0)
		return fp;

	// mpfps が必ずあるとは限らないらしい。
	return 0;
}



}  // namespace


mp_floating_pointer_structure* g_mpfps;
int cpu_init()
{
	/*
	// setup.S で設定される

	u64 reg;
	reg = get_cr4_64();
	// clear VME flag
	// 仮想8086モード割り込みを無効化する
	reg &= ~0x0000000000000001;
	set_cr4_64(reg);
	*/

	cpu0_gdt.null_entry.set_null();
	cpu0_gdt.kern_code.set(0);
	cpu0_gdt.kern_data.set(0);
	cpu0_gdt.user_code.set(3);
	cpu0_gdt.user_data.set(3);
	native::gdt_ptr64 gdtptr;
	gdtptr.set(sizeof cpu0_gdt, &cpu0_gdt);
	native::lgdt(&gdtptr);
	native::set_ss(cpu0_gdt.kern_data_offset());


	mp_floating_pointer_structure* mpfps = search_mpfps();
	g_mpfps = mpfps;
	mp_configuration_table_header* mpcth =
	    (mp_configuration_table_header*)
	    (arch::PHYSICAL_MEMMAP_BASEADR + mpfps->mp_config_padr);
	const u8* entry = mpcth->first_entry();
	u8 cpus = 0;
	for (u16 i = 0; i < mpcth->entry_count; i++) {
		switch (*entry) {
		case 0: {
			processor_entry* pe = (processor_entry*)entry;
			processor[cpus].id = pe->local_apic_id;
			++cpus;
			entry = pe->next_entry();
			break;
		}
		case 1: {
			bus_entry* be = (bus_entry*)entry;
			entry = be->next_entry();
			tmp[0]++;
			break;
		}
		case 2: {
			io_apic_entry* ae = (io_apic_entry*)entry;
			entry = ae->next_entry();
			tmp[1]++;
			break;
		}
		case 3: {
			io_int_entry* ie = (io_int_entry*)entry;
			entry = ie->next_entry();
			tmp[2]++;
			break;
		}
		case 4: {
			local_int_entry* le = (local_int_entry*)entry;
			entry = le->next_entry();
			tmp[3]++;
			break;
		}
		}
	}
	cpu_count = cpus;

	intr_init();

	//pic_init();

	return 0;
}

void cpu_test()
{
	u8* p;
	setup_get_mp_info(&p);
	log()("p=")(p)();
	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 16; i++) {
			log()(" ").u(p[j*16+i], 16);
		}
		log()();
	}

	log()("mpfps=")(g_mpfps)();
	log()("mpfps->features1=").u(g_mpfps->features1)();
	log()("mpcth=").u(g_mpfps->mp_config_padr, 16)();

	p = (u8*)0xfdbe0;
	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 16; i++) {
			log()(" ").u(p[j*16+i], 16);
		}
		log()();
	}

	mp_configuration_table_header* mpcth =
	    (mp_configuration_table_header*)
	    (arch::PHYSICAL_MEMMAP_BASEADR + g_mpfps->mp_config_padr);

	log()("mpcth->test()=").u((u8)mpcth->test())();
	log()("mpcth->signature=").u(mpcth->signature)();
	log()("mpcth->entry_count=").u(mpcth->entry_count)();

	for (u32 i = 0; i < cpu_count; i++) {
		log()("cpu").u(i,10)("=").u(processor[i].id)();
	}
}

namespace arch {

void halt() {
	native::hlt();
}

}  // namespace arch
