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
#include <core/io_node.hh>
#include <core/setup.hh>
#include <core/mempool.hh>
#include <core/string.hh>
#include <new_ops.hh>


namespace {

uptr filename_length(const char* path)
{
	uptr r = 0;
	while (*path) {
		if (*path == '/')
			break;
		++path;
	}

	return r;
}

int filename_compare(const char* name1, const char* name2)
{
	for (;;) {
		if (*name1 != *name2) {
			if ((*name1 == '\0' || *name1 == '/') &&
			    (*name2 == '\0' || *name2 == '/'))
				return 0;
			return *name1 - *name2;
		}
		if (*name1 == '\0' || *name1 == '/')
			return 0;

		++name1;
		++name2;
	}
}

bool pathname_is_edge(const char* path)
{
	if (!*path)
		return false;

	for (;; ++path) {
		if (*path == '\0')
			return true;
		if (*path == '/')
			break;
	}

	while (*path == '/')
		++path;

	return *path == '\0';
}

}  // namespace

// fs_ctl::mountpoint

fs_ctl::mountpoint::mountpoint(const char* src, const char* tgt) :
	mount_obj(nullptr),
	down_mountpoint(nullptr)
{
	source_char_cn = str_length(src);
	target_char_cn = str_length(tgt);

	pathname_cn_t src_len = src ? source_char_cn + 1 : 0;
	pathname_cn_t tgt_len = tgt ? target_char_cn + 1 : 0;

	str_copy(src_len, src, &buf[0]);
	str_copy(tgt_len, tgt, &buf[src_len]);

	source = src ? &buf[0] : nullptr;
	target = tgt ? &buf[src_len] : nullptr;
}

cause::pair<fs_ctl::mountpoint*> fs_ctl::mountpoint::create(
    const char* source, const char* target)
{
	uptr size = sizeof (mountpoint);
	size += source ? str_length(source) + 1 : 0;
	size += target ? str_length(target) + 1 : 0;

	mountpoint* mp = new (mem_alloc(size)) mountpoint(source, target);
	if (!mp)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, mp);
}

cause::t fs_ctl::mountpoint::destroy(mountpoint* mp)
{
	mp->~mountpoint();
	operator delete (mp, mp);
	mem_dealloc(mp);

	return cause::OK;
}

// fs_ctl

fs_ctl::fs_ctl()
{
}

cause::t fs_ctl::init()
{
	fs_node* _root = new (shared_mem())
	    fs_node(nullptr, fs_ctl::NODE_UNKNOWN);
	if (!_root)
		return cause::NOMEM;

	root = _root;

	return cause::OK;
}

cause::pair<io_node*> fs_ctl::open(const char* path, u32 flags)
{
	fs_node* cur = root;
	while (*path) {
		while (*path == '/')
			++path;

		auto child = cur->get_child_node(path);
		if (is_fail(child)) {
			if ((flags & OPEN_CREATE) && pathname_is_edge(path)) {
				//auto r = cur->create_child_node(cur, path);
			}
		} else if (is_fail(child)) {
			return cause::null_pair(child.get_cause());
		}

		cur = child.get_data();

		uptr len = filename_length(path);
		path += len;
	}

	if (cur) {}

	return null_pair(cause::NOFUNC);
}

cause::t fs_ctl::mount(const char* source, const char* target, const char* type)
{
	if (str_compare(32, "ramfs", type) == 0) {
		auto r1 = mountpoint::create(source, target);
		if (is_fail(r1))
			return r1.get_cause();

		mountpoint* mp = r1.get_data();

		auto r2 = ramfs_driver->mount(source);
		if (is_fail(r2)) {
			auto r3 = mountpoint::destroy(mp);
			if (is_fail(r3)) {
				// TODO:multierror
			}
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

cause::t fs_ctl::unmount(const char* target, u64 flags)
{
	mountpoint* target_mp = nullptr;
	for (mountpoint* mp = mountpoints.back();
	     mp;
	     mp = mountpoints.prev(mp))
	{
		if (str_compare(target, mp->target) == 0) {
			target_mp = mp;
			break;
		}
	}

	if (!target_mp)
		return cause::BADARG;

	mountpoints.remove(target_mp);

	auto r = target_mp->mount_obj->get_driver()->unmount(
	    target_mp->mount_obj, target_mp->target, flags);

	return r;
}

// TODO:ファイル名の手前のディレクトリまでを返す関数を作る

cause::pair<fs_node*> fs_ctl::get_fs_node(
    fs_node* base, const char* path, u32 flags)
{
	if (base == nullptr)
		base = root;

	for (;;) {
		while (*path == '/')
			++path;

		auto child = base->get_child_node(path);

		uptr len = filename_length(path);
		path += len;
	}
}


// fs_driver::operations

void fs_driver::operations::init()
{
	Mount    = fs_driver::nofunc_Mount;
	Unmount  = fs_driver::nofunc_Unmount;
}


fs_ctl* get_fs_ctl()
{
	return global_vars::core.fs_ctl_obj;
}

cause::t fs_ctl_init()
{
	fs_ctl* fsctl = new (shared_mem()) fs_ctl;
	if (!fsctl)
		return cause::NOMEM;

	auto r = fsctl->init();
	if (is_fail(r)) {
		// TODO:logging return value
		new_destroy(fsctl, shared_mem());
		return r;
	}

	global_vars::core.fs_ctl_obj = fsctl;

	return cause::OK;
}

// fs_mount::operations

void fs_mount::operations::init()
{
	CreateNode   = fs_mount::nofunc_CreateNode;
	OpenNode     = fs_mount::nofunc_OpenNode;
}


// fs_node

fs_node::fs_node(fs_mount* mount_owner, u32 _mode) :
	owner(mount_owner),
	mode(_mode)
{
}

cause::pair<fs_node*> fs_node::create_child_node(u32 mode, const char* name)
{
	return owner->create_node(this, mode, name);
}

cause::pair<fs_node*> fs_node::get_child_node(const char* name)
{
	child_node* child;
	for (child = child_nodes.front(); child; child_nodes.next(child)) {
		if (0 == filename_compare(child->name, name)) {
			break;
		}
	}

	if (child)
		return cause::make_pair(cause::OK, child->node);
	else
		return cause::null_pair(cause::NOENT);
}


cause::t sys_mount(
    const char* source,  ///< source device path
    const char* target,  ///< mount target path
    const char* type,    ///< fs type name
    u64 flags,           ///< mount flags
    const void* data)    ///< optional data
{
	return get_fs_ctl()->mount(source, target, type);
}

cause::t sys_unmount(
    const char* target,  ///< unmount taget path
    u64 flags)
{
	return get_fs_ctl()->unmount(target, flags);
}
