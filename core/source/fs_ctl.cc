/// @file  fs_ctl.cc
/// @brief filesystem controller.

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include <core/fs_ctl.hh>

#include <core/global_vars.hh>
#include <core/setup.hh>
#include <core/mempool.hh>
#include <core/string.hh>
#include <new_ops.hh>


// fs_ctl::mountpoint

fs_ctl::mountpoint::mountpoint(const char* src, const char* tgt) :
	mount_obj(nullptr)
{
	uptr src_len = str_length(src) + 1;
	uptr tgt_len = str_length(tgt) + 1;

	str_copy(src_len, src, &buf[0]);
	str_copy(tgt_len, tgt, &buf[src_len]);

	source = &buf[0];
	target = &buf[src_len];
}

cause::pair<fs_ctl::mountpoint*> fs_ctl::mountpoint::create(
    const char* source, const char* target)
{
	uptr size = sizeof (mountpoint);
	size += str_length(source) + 1;
	size += str_length(target) + 1;

	mountpoint* mp = new (mem_alloc(size)) mountpoint(source, target);
	if (!mp)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, mp);
}

// fs_ctl

fs_ctl::fs_ctl()
{
}

cause::t fs_ctl::init()
{
	fs_node* _root = new (mem_alloc(sizeof (fs_node))) fs_node;
	if (!_root)
		return cause::NOMEM;

	root = _root;

	return cause::OK;
}

cause::t fs_ctl::mount(const char* type, const char* source, const char* target)
{
	if (str_compare(32, "ramfs", type)) {
		auto r1 = mountpoint::create(source, target);
		if (is_fail(r1))
			return r1.get_cause();

		mountpoint* mp = r1.get_data();

		auto r2 = ramfs_driver->mount(source);
		if (is_fail(r2)) {
			mp->~mountpoint();
			operator delete (mp, (void*)nullptr);
			mem_dealloc(mp);
			return r2.get_cause();
		}

		mp->mount_obj = r2.get_data();

		mountpoints.push_back(mp);

		return cause::OK;
	}
	else {
		return cause::NODEV;
	}
}


// fs_driver::operations

void fs_driver::operations::init()
{
	mount = fs_driver::nofunc_fs_driver_mount;
}

fs_ctl* get_fs_ctl()
{
	return global_vars::core.fs_ctl_obj;
}

cause::t fs_ctl_init()
{
	fs_ctl* fsctl = new (mem_alloc(sizeof (fs_ctl))) fs_ctl;
	if (!fsctl)
		return cause::NOMEM;

	auto r = fsctl->init();
	if (is_fail(r)) {
		fsctl->~fs_ctl();
		operator delete (fsctl, (void*)nullptr);
		mem_dealloc(fsctl);
		return r;
	}

	global_vars::core.fs_ctl_obj = fsctl;

	return cause::OK;
}

