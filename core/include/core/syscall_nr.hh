/// @file  core/syscall_nr.hh
/// @brief System call number definition.

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

#ifndef CORE_SYSCALL_NR_HH_
#define CORE_SYSCALL_NR_HH_


enum SYSCALL_NR
{
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_WRITE,
    SYSCALL_READ,
    SYSCALL_MKDIR,
    SYSCALL_READENTS,
    SYSCALL_MOUNT,

    SYSCALL_NR,
};


#endif  // CORE_SYSCALL_NR_HH_

