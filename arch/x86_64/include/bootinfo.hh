/// @file  bootinfo.hh
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_BOOTINFO_HH_
#define ARCH_X86_64_INCLUDE_BOOTINFO_HH_

#include <basic.hh>
#include <multiboot2.h>


namespace bootinfo {

struct tag;

struct header
{
	u32 length;
	u32 zero;

	const tag* next() const {
		return (const tag*)(this + 1);
	}
};

enum BOOTINFO_TYPE {
	TYPE_ADR_MAP = 1,
	TYPE_MEM_ALLOC,
	TYPE_LOG,
	TYPE_BUNDLE,
	TYPE_MULTIBOOT,

	TYPE_END = 0xff,
};

struct tag
{
	u16 type;   ///< BOOTINFO_TYPE
	u16 flags;
	u32 size;

	const tag* next() const {
		return (const tag*)((const u8*)this + size);
	}
};

struct adr_map : tag
{
	struct entry
	{
		enum TYPE {
			AVAILABLE = 1,
			RESERVED,
			ACPI,
			NVS,
			BADRAM,
		};
		u64 adr;
		u64 len;
		u32 type;   ///< TYPE
		u32 zero;
	};

	entry entries[0];
};

struct mem_alloc_entry {
	u64 adr;
	u64 bytes;
};
struct mem_alloc {
	u32 type;
	u32 size;
	mem_alloc_entry entries[0];
};

struct module : tag
{
	u32 mod_start;
	u32 mod_size;
	char cmdline[0];
};

struct log : tag
{
	u8 log[0];
};

struct multiboot : tag
{
	enum {
		FLAG_2 = 1,  ///< multiboot2 if set.
	};

	u8 info[0];
};

enum {
	MAX_BYTES = 0x60000,

	// Available memory end during kernel boot.
	BOOTHEAP_END   = 0x01ffffff,
};

const tag* get_info(u16 type);

}  // namespace bootinfo


#endif  // include guard

