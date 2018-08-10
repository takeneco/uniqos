/// @file   sys_fs.cc
/// @brief  Filesystem system calls.

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

#include <core/sys_fs.hh>


namespace uniqos {

cause::pair<ucpu> sys_open(
    const char* path,
    u32 flags)
{
    fs_ctl* fsctl = get_fs_ctl();
    process* proc = get_current_process();
    spin_wlock_section proc_lock(proc->ref_lock());

    auto pathnodes = fs::path_parser::create(proc, path);
    if (is_fail(pathnodes))
        return zero_pair(pathnodes.cause());

    fs_node* target = pathnodes->get_edge_fsnode();
    if (!target) {
        fs_node* _parent = pathnodes->get_edge_parent_fsnode();
        if (!_parent->is_dir()) {
            fs::path_parser::destroy(pathnodes);
            return zero_pair(cause::NOTDIR);
        }

        fs_dir_node* parent = static_cast<fs_dir_node*>(_parent);
        auto child = parent->create_child_reg_node(
            pathnodes->get_edge_name());
        if (is_fail(child)) {
            fs::path_parser::destroy(pathnodes);
            return zero_pair(child.cause());
        }

        target = child.value();
    }

    auto ion = target->open(flags);

    fs::path_parser::destroy(pathnodes);

    /*
    auto ion = fsctl->open_node(fsns, cwd, path, flags);
    */
    if (is_fail(ion)) {
        return zero_pair(ion.cause());
    }

    auto iod = proc->append_io_desc(ion.value(), 0);
    if (is_fail(iod)) {
        return zero_pair(iod.cause());
    }

    return cause::pair<ucpu>(cause::OK, iod.value());
}

cause::pair<ucpu> sys_close(int iod)
{
    process* pr = get_current_process();

    auto iodesc = pr->get_io_desc(iod);
    if (is_fail(iodesc))
        return zero_pair(iodesc.cause());

    io_node::close(iodesc.value()->io);

    cause::t r = pr->clear_io_desc(iod);
    if (is_fail(r))
        return zero_pair(r);

    return zero_pair(cause::OK);
}

cause::pair<ucpu> sys_write(int iod, const void* buf, uptr bytes)
{
    auto _desc = get_current_process()->get_io_desc(iod);
    if (is_fail(_desc))
        return zero_pair(_desc.cause());

    io_desc* desc = _desc.value();

    auto off = desc->io->write(desc->off, buf, bytes);

    desc->off += off.value();

    return off;
}

cause::pair<ucpu> sys_read(int iod, void* buf, uptr bytes)
{
    auto _desc = get_current_process()->get_io_desc(iod);
    if (is_fail(_desc))
        return zero_pair(_desc.cause());

    io_desc* desc = _desc.value();

    auto off = desc->io->read(desc->off, buf, bytes);

    desc->off += off.value();

    return off;
}

cause::pair<ucpu> sys_mkdir(const char* path)
{
    return zero_pair(get_fs_ctl()->mkdir(get_current_process(), path));
}

cause::pair<ucpu> sys_readents(int iod, uptr bytes, void* buf)
{
    process* proc = get_current_process();

    auto iodesc = proc->get_io_desc(iod);
    if (is_fail(iodesc))
        return zero_pair(iodesc.cause());

    auto r = iodesc->io->read_entry(
        &iodesc->off, bytes, static_cast<io_node_entry*>(buf));

    return cause::pair<ucpu>(r.cause(), r.value());
}

cause::pair<ucpu> sys_mount(
    const char* source,  ///< source device.
    const char* target,  ///< mount target path.
    const char* type,    ///< fs type name.
    u32         flags,   ///< mount flags.
    const void* data)    ///< optional data.
{
    process* proc = get_current_process();

    cause::t r = get_fs_ctl()->mount(proc, source, target, type, flags, data);

    return zero_pair(r);
}

cause::pair<ucpu> sys_unmount(
    const char* target,  ///< unmount taget path
    u64 flags)
{
    process* proc = get_current_process();

    cause::t r = get_fs_ctl()->unmount(proc, target, flags);

    return zero_pair(r);
}

}  // namespace uniqos

