/// @file   sys_fs.cc
/// @brief  Filesystem syscalls.

//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <core/process.hh>
#include <core/fs_ctl.hh>


cause::t sys_mount(
    const char* source,  ///< source device.
    const char* target,  ///< mount target path.
    const char* type,    ///< fs type name.
    u64         flags,   ///< mount flags.
    const void* data)    ///< optional data.
{
	process* proc = get_current_process();

	return get_fs_ctl()->mount(proc, source, target, type, flags, data);
}

cause::t sys_unmount(
    const char* target,  ///< unmount taget path
    u64 flags)
{
	process* proc = get_current_process();

	return get_fs_ctl()->unmount(proc, target, flags);
}

