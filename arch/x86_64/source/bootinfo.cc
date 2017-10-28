/// @file   bootinfo.cc
/// @brief  Access to setup data.

//  Uniqos  --  Unique Operating System
//  (C) 2010 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <bootinfo.hh>

#include <core/ctype.hh>
#include <global_vars.hh>
#include <util/string.hh>


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

/// @brief  Search "key=value" from kernel command line.
/// @param[in] key  Null terminated string of key.
/// @param[in] value_bytes  Byte size of value.
/// @param[out] value  Null terminated value destination if "key=value" found.
/// @return  This function returns byte size without '\0' of value as
///   pair::value.
///   pair::cause is cause::OK if key is found and is cause::FAIL if key is
///   not found.
cause::pair<uptr> get_cmdline_value(
    const char* key, uptr value_bytes, char* value)
{
	const char* cmd = get_cmdline();
	int keylen = str_length(key);

	for (;;) {
		if (str_startswith(cmd, key) && cmd[keylen] == '=') {
			cmd += keylen + 1;
			break;
		}
		while (*cmd && !ctype::is_space(*cmd))
			++cmd;
		while (*cmd && ctype::is_space(*cmd))
			++cmd;
		if (!*cmd)
			return zero_pair(cause::FAIL);
	}

	uptr vallen;
	for (vallen = 0; vallen < value_bytes; ++vallen) {
		if (!cmd[vallen] || ctype::is_space(cmd[vallen])) {
			value[vallen] = '\0';
			break;
		}
		value[vallen] = cmd[vallen];
	}

	while (cmd[vallen] && !ctype::is_space(cmd[vallen]))
		++vallen;

	return make_pair(cause::OK, vallen);
}

}  // namespace bootinfo

