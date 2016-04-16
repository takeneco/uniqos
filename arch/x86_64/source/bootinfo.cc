/// @file   bootinfo.cc
/// @brief  Access to setup data.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2013 KATO Takeshi
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

#include <bootinfo.hh>

#include <global_vars.hh>


namespace bootinfo {

const tag* get_info(u16 type)
{
	const void* const bi = global_vars::arch.bootinfo;

	const header* h = static_cast<const header*>(bi);

	const void* const end = h->end();

	const tag* info = h->next_tag();

	while (info->info_type != TYPE_END && info < end) {
		if (info->info_type == type)
			return info;

		info = info->next_tag();
	}

	return 0;
}

// multiboot.hとmultiboot2.hは同時にincludeできないのでmultiboot.hに対応する
// ときにここから先を別ファイルにする必要がある。
#include <multiboot2.h>

/// @return This function returns boot command line string with null terminated.
///         If no boot command line, this function returns nullptr.
const char* get_cmdline()
{
	const multiboot* mb =
	    static_cast<const multiboot*>(get_info(TYPE_MULTIBOOT));
	if (!mb)
		return nullptr;

	const u8* p = mb->info;
	u32 read = 0;

	const u32 total_size = *reinterpret_cast<const u32*>(p);
	read += 8;
	p += 8;

	while (read < total_size) {
		const multiboot_tag* mbtag =
		     reinterpret_cast<const multiboot_tag*>(p);
		if (mbtag->type == MULTIBOOT_TAG_TYPE_CMDLINE) {
			const multiboot_tag_string* mb_cmdline =
			    reinterpret_cast<const multiboot_tag_string*>(p);
			return mb_cmdline->string;
		}
		else if (mbtag->type == MULTIBOOT_TAG_TYPE_END) {
			break;
		}

		const u32 sz = up_align<u32>(mbtag->size, MULTIBOOT_TAG_ALIGN);
		read += sz;
		p += sz;
	}

	return nullptr;
}

}  // namespace bootinfo

