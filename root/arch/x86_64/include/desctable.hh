// @file   desctable.hh
// @brief  GDT, TSS
//
// (C) 2010 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_DESCTABLE_HH_
#define ARCH_X86_64_INCLUDE_DESCTABLE_HH_


enum {
	GDT_KERN_CODESEG = 1,
	GDT_KERN_DATASEG = 2,
	GDT_USER_CODESEG = 3,
	GDT_USER_DATASEG = 4,
};


namespace arch
{

struct tss64
{
	u32 reserved1;  // 0x00

	u32 rsp0_l;     // 0x04
	u32 rsp0_h;
	u32 rsp1_l;     // 0x0c
	u32 rsp1_h;
	u32 rsp2_l;     // 0x14
	u32 rsp2_h;

	u32 reserved2;  // 0x1c
	u32 reserved3;

	u32 ist1_l;     // 0x24
	u32 ist1_h;
	u32 ist2_l;     // 0x2c
	u32 ist2_h;
	u32 ist3_l;     // 0x34
	u32 ist3_h;
	u32 ist4_l;     // 0x3c
	u32 ist4_h;
	u32 ist5_l;     // 0x44
	u32 ist5_h;
	u32 ist6_l;     // 0x4c
	u32 ist6_h;
	u32 ist7_l;     // 0x54
	u32 ist7_h;

	u32 reserved4;  // 0x5c
	u32 reserved5;

	u16 reserved6;  // 0x64
	u16 ioperm_map_offset;

	u8  intr_redir_map[32];

	void init_iomap_offset() {
		iomap_offset = sizeof (tss64);
	}
};

/// LDT and TSS descriptor
class ldttss_desc
{
protected:
	typedef u64 type;
	type e[2];

public:
	enum {
		LDT = U64CAST(0x2) << 40,
		TSS = U64CAST(0x9) << 40,

		P   = U64CAST(1) << 47,  ///< Descriptor exist if set.
		G   = U64CAST(1) << 55,  ///< Limit scale 4096 times if set.
	};

	void set_null() {
		e[0] = e[1] = 0;
	}
	void set(type base, type limit, type dpl, type flags) {
		e[0] = (base  & 0x00ffffff) << 16 |
		       (base  & 0xff000000) << 32 |
		       (limit & 0x0000ffff)       |
		       (limit & 0x000f0000) << 32 |
		       (dpl & 3) << 45 |
		       flags;
		e[1] = base & U64CAST(0xffffffff00000000) >> 32;
	}
};

typedef ldttss_desc tss_desc;
typedef ldttss_desc ldt_desc;


}  // namespace arch


#endif  // Include guard.
