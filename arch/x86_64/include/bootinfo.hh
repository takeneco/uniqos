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

	const tag* next_tag() const {
		return (const tag*)(this + 1);
	}
	const void* end() const {
		return (const u8*)this + length;
	}
};

enum INFO_TYPE {
	TYPE_END = 0,

	TYPE_ADR_MAP = 1,
	TYPE_MEM_ALLOC,
	TYPE_LOG,
	TYPE_BUNDLE,
	TYPE_MULTIBOOT,
};

struct tag
{
	u16 info_type;   ///< BOOTINFO_TYPE
	u16 info_flags;
	u32 info_bytes;

	const tag* next_tag() const {
		return (const tag*)((const u8*)this + info_bytes);
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
		u64 bytes;
		u32 type;   ///< TYPE
		u32 zero;
	};

	const void* end_entry() const {
		return tag::next_tag();
	}

	entry entries[0];
};

struct mem_alloc : tag
{
	struct entry
	{
		u64 adr;
		u64 bytes;
	};

	const void* end_entry() const {
		return tag::next_tag();
	}

	entry entries[0];
};

struct module : tag
{
	u32 mod_start;
	u32 mod_bytes;
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

