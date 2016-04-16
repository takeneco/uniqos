/// @file  bootinfo.hh

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2014 KATO Takeshi
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

#ifndef ARCH_X86_64_INCLUDE_BOOTINFO_HH_
#define ARCH_X86_64_INCLUDE_BOOTINFO_HH_

#include <core/basic.hh>


namespace bootinfo {

enum {
	ALIGN = 8,
};

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

	/// Hardware implemented memory information
	TYPE_ADR_MAP = 1,

	/// Allocated memory information
	TYPE_MEM_ALLOC,

	/// Boot thread workspace memory information
	/// @detail  This memory can be free after boot thread finished.
	TYPE_MEM_WORK,

	///< Boot log contents
	TYPE_LOG,

	///< Bundled module
	TYPE_BUNDLE,

	///< Multiboot information
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

struct _mem_list : tag
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

typedef _mem_list mem_alloc;

typedef _mem_list mem_work;

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
const char* get_cmdline();

}  // namespace bootinfo


#endif  // include guard

