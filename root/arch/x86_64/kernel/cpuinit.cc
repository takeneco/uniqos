/// @file   cpuinit.cc
/// @brief  Initialize GDT/IDT.
//
// (C) 2010 KATO Takeshi
//

#include "kerninit.hh"

#include "btypes.hh"
#include "desctable.hh"
#include "native_ops.hh"


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

}  // namespace


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

	/*
	gdt[0].set_null();
	gdt[GDT_KERN_CODESEG].set(0, 0, 0,
	    gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);
	gdt[GDT_KERN_DATASEG].set(0, 0, 0,
	    gdte::RW | gdte::S | gdte::P | gdte::L | gdte::G);
	gdt[GDT_USER_CODESEG].set(0, 0, 3,
	    gdte::XR | gdte::S | gdte::P | gdte::L | gdte::G);
	gdt[GDT_USER_DATASEG].set(0, 0, 3,
	    gdte::RW | gdte::S | gdte::P | gdte::L | gdte::G);

	native::gdt_ptr64 gdtptr;
	gdtptr.init(sizeof gdt, gdt);
	native::lgdt(&gdtptr);

	native::set_ss(8 * GDT_KERN_DATASEG);
	*/
	cpu0_gdt.null_entry.set_null();
	cpu0_gdt.kern_code.set(0);
	cpu0_gdt.kern_data.set(0);
	cpu0_gdt.user_code.set(3);
	cpu0_gdt.user_data.set(3);
	native::gdt_ptr64 gdtptr;
	gdtptr.init(sizeof cpu0_gdt, &cpu0_gdt);
	native::lgdt(&gdtptr);
	native::set_ss(cpu0_gdt.kern_data_offset());

//	dump()("EBDA = ").u(*(u16*)(arch::PHYSICAL_MEMMAP_BASEADR + 0x40e), 16)();

	intr_init();

	//pic_init();

	return 0;
}
