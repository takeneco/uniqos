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
	const void* const bootinfo = global_vars::arch.bootinfo;

	const header* h =
	    static_cast<const header*>(bootinfo);

	const void* const end =
	    static_cast<const u8*>(bootinfo) + h->length;

	const tag* info = h->next();
	while (info->type != TYPE_END && info < end) {
		if (info->type == type)
			return info;

		info = info->next();
	}

	return 0;
}
/*
const void* get_bootinfo(u32 tag_type)
{
	const u8* info =
	    static_cast<const u8*>(global_vars::arch.bootinfo);

	const u32 total_size = *reinterpret_cast<const u32*>(info);

	u32 read = sizeof (u32) * 2;

	for (;;) {
		const multiboot_tag* tag =
		    reinterpret_cast<const multiboot_tag*>(&info[read]);

		if (tag->type == tag_type)
			return tag;

		read += up_align<u32>(tag->size, MULTIBOOT_TAG_ALIGN);
		if (read >= total_size)
			break;
	}

	return 0;
}*/

}  // namespace bootinfo

