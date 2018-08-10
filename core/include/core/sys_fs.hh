/// @file  core/sys_fs.hh
/// @brief  Filesystem syscall declarations.

//  Uniqos  --  Unique Operating System
//  (C) 2018 KATO Takeshi
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

#ifndef CORE_SYS_FS_HH_
#define CORE_SYS_FS_HH_

#include <core/basic-types.hh>


namespace uniqos {

cause::pair<ucpu> sys_open(
    const char* path,
    u32 flags);

cause::pair<ucpu> sys_close(
    int iod);

cause::pair<ucpu> sys_write(
    int iod,
    const void* buf,
    uptr bytes);

cause::pair<ucpu> sys_read(
    int iod,
    void* buf,
    uptr bytes);

cause::pair<ucpu> sys_mkdir(
    const char* path);

cause::pair<ucpu> sys_readents(
    int fd,
    uptr bytes,
    void* buf);

cause::pair<ucpu> sys_mount(
    const char* source,
    const char* target,
    const char* type,
    u32 flags,
    const void* data);

}  // namespace uniqos


#endif  // CORE_SYS_FS_HH_

