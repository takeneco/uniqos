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

}  // namespace bootinfo

