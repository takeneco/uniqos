/// @file   core/source/syscall_entry.cc
/// @brief  System call core entry point.

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

#include <core/syscall_wrap.hh>
#include <core/syscall_nr.hh>

#include <core/global_vars.hh>
#include <core/new_ops.hh>
#include <core/sys_fs.hh>


namespace uniqos {

using syscall_func = cause::pair<ucpu> (*)(const ucpu[6]);

class syscall_ctl
{
public:
    void init();

    static const syscall_func map[SYSCALL_NR];
};

const syscall_func syscall_ctl::map[] = {

[SYSCALL_OPEN] = syscall_wrap2<
    const char*, u32, sys_open>,

[SYSCALL_CLOSE] = syscall_wrap1<
    int, sys_close>,

[SYSCALL_WRITE] = syscall_wrap3<
    int, const void*, uptr, sys_write>,

[SYSCALL_READ] = syscall_wrap3<
    int, void*, uptr, sys_read>,

[SYSCALL_MKDIR] = syscall_wrap1<
    const char*, sys_mkdir>,

[SYSCALL_READENTS] = syscall_wrap3<
    int, uptr, void*, sys_readents>,

[SYSCALL_MOUNT] = syscall_wrap5<
    const char*, const char*, const char*, u32, const void*, sys_mount>,

};

void syscall_ctl::init()
{
}

cause::t syscall_setup()
{
    syscall_ctl* ctl = new (generic_mem()) syscall_ctl;
    if (!ctl)
        return cause::NOMEM;

    ctl->init();

    global_vars::core.syscall_ctl_obj = ctl;

    return cause::OK;
}

syscall_ctl* get_syscall_ctl()
{
    return global_vars::core.syscall_ctl_obj;
}

cause::pair<uptr> core_syscall_entry(const ucpu* data)
{
    return get_syscall_ctl()->map[data[0]](data + 1);
}

}  // namespace uniqos

