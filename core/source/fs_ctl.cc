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
#include <core/mempool.hh>
#include <core/setup.hh>
#include <core/string.hh>


namespace {

uptr nodename_length(const char* path)
{
	uptr r = 0;
	while (*path) {
		if (*path == '/')
			break;
		++path;
	}

	return r;
}

int nodename_compare(const char* name1, const char* name2)
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

bool nodename_is_file(const char* path)
{
	if (!*path)
		return false;

	for (;; ++path) {
		if (*path == '\0')
			return true;
		if (*path == '/')
			return false;
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

// fs_mount::operations

void fs_mount::operations::init()
{
	CreateNode   = fs_mount::nofunc_CreateNode;
	OpenNode     = fs_mount::nofunc_OpenNode;
}


// mount_info

mount_info::mount_info(const char* src, const char* tgt) :
	mount_obj(nullptr)
{
	source_char_cn = str_length(src);
	target_char_cn = str_length(tgt);

	pathname_cn_t src_len = src ? source_char_cn + 1 : 0;
	pathname_cn_t tgt_len = tgt ? target_char_cn + 1 : 0;

	str_copy(src, &buf[0], src_len);
	str_copy(tgt, &buf[src_len], tgt_len);

	source = src ? &buf[0] : nullptr;
	target = tgt ? &buf[src_len] : nullptr;
}

cause::pair<mount_info*> mount_info::create(
    const char* source, const char* target)
{
	uptr size = sizeof (mount_info);
	size += source ? str_length(source) + 1 : 0;
	size += target ? str_length(target) + 1 : 0;

	mount_info* mp = new (mem_alloc(size)) mount_info(source, target);
	if (!mp)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, mp);
}

cause::t mount_info::destroy(mount_info* mp)
{
	mp->~mount_info();
	operator delete (mp, mp);
	mem_dealloc(mp);

	return cause::OK;
}

// fs_node

fs_node::fs_node(fs_mount* mount_owner, u32 _mode) :
	owner(mount_owner),
	mode(_mode)
{
}

bool fs_node::is_dir() const
{
	return mode == fs_ctl::NODE_DIR;
}

bool fs_node::is_regular() const
{
	return mode == fs_ctl::NODE_REG;
}

void fs_node::insert_mount(mount_info* mi)
{
	mounts.push_front(mi);
}

void fs_node::remove_mount(mount_info* mi)
{
	mounts.remove(mi);
}

cause::pair<fs_node*> fs_node::create_child_node(u32 mode, const char* name)
{
	return owner->create_node(this, mode, name);
}

cause::pair<fs_node*> fs_node::get_child_node(const char* name)
{
	child_node* child;
	for (child = child_nodes.front(); child; child_nodes.next(child)) {
		if (0 == nodename_compare(child->name, name)) {
			break;
		}
	}

	if (child)
		return cause::make_pair(cause::OK, child->node);
	else
		return cause::null_pair(cause::NOENT);
}

cause::t fs_node::append_child_node(const char* name, fs_node* fsn)
{
	child_node* child = new (mem_alloc(child_node::calc_size(name)))
	    child_node;
	if (!child)
		return cause::NOMEM;

	child->node = fsn;
	str_copy(name, child->name);

	return cause::OK;
}

// fs_node::child_node

uptr fs_node::child_node::calc_size(const char* name)
{
	return sizeof (child_node) + str_length(name) + 1;
}


// fs_rootfs_drv

fs_rootfs_drv::fs_rootfs_drv() :
	fs_driver(&fs_driver_ops)
{
	fs_driver_ops.init();
}


// fs_rootfs_mnt

fs_rootfs_mnt::fs_rootfs_mnt(fs_rootfs_drv* drv) :
	fs_mount(drv, &fs_mount_ops),
	root_node(this, fs_ctl::NODE_DIR)
{
	fs_mount_ops.init();

	root = &root_node;
}


// fs_ctl

fs_ctl::fs_ctl() :
	rootfs_mnt(&rootfs_drv)
{
}

cause::t fs_ctl::setup()
{
	root = rootfs_mnt.get_root_node();

	return cause::OK;
}

cause::pair<io_node*> fs_ctl::open(const char* path, u32 flags)
{
	fs_node* cur = root;

	while (*path) {
		while (*path == '/')
			++path;

		if (pathname_is_edge(path))
			break;

		auto child = cur->get_child_node(path);
		if (is_fail(child))
			return null_pair(child.cause());
		cur = child.data();

		uptr len = nodename_length(path);
		path += len;
	}

	auto target_node = cur->get_child_node(path);
	if (is_fail(target_node)) {
		if ((flags & OPEN_CREATE) && nodename_is_file(path)) {
			target_node = cur->create_child_node(0, path);
			if (is_fail(target_node))
				return null_pair(target_node.cause());
		} else {
			return null_pair(target_node.cause());
		}
	}

	return target_node.data()->open(flags);
}

cause::t fs_ctl::mount(const char* source, const char* target, const char* type)
{
	if (str_compare(32, "ramfs", type) == 0) {
		auto target_node = get_fs_node(nullptr, target, 0);
		if (is_fail(target_node))
			return target_node.cause();

		auto r1 = mount_info::create(source, target);
		if (is_fail(r1))
			return r1.cause();

		mount_info* mp = r1.data();

		auto r2 = ramfs_driver->mount(source);
		if (is_fail(r2)) {
			auto r3 = mount_info::destroy(mp);
			if (is_fail(r3)) {
				// TODO:multierror
			}
			return r2.cause();
		}

		mp->mount_obj = r2.data();

		target_node.data()->insert_mount(mp);

		mountpoints.push_front(mp);

		return cause::OK;
	}
	else {
		return cause::NODEV;
	}
}

cause::t fs_ctl::unmount(const char* target, u64 flags)
{
	mount_info* target_mp = nullptr;
	for (mount_info* mp = mountpoints.front();
	     mp;
	     mp = mountpoints.next(mp))
	{
		if (str_compare(target, mp->target) == 0) {
			target_mp = mp;
			break;
		}
	}

	if (!target_mp)
		return cause::BADARG;

	auto target_node = get_fs_node(nullptr, target, 0);
	if (is_fail(target_node))
		return target_node.cause();

	mountpoints.remove(target_mp);

	target_node.data()->remove_mount(target_mp);

	auto r = target_mp->mount_obj->get_driver()->unmount(
	    target_mp->mount_obj, target_mp->target, flags);

	mount_info::destroy(target_mp);

	return r;
}

// TODO:ファイル名の手前のディレクトリまでを返す関数を作れば使える

cause::pair<fs_node*> fs_ctl::get_fs_node(
    fs_node* base, const char* path, u32 flags)
{
	fs_node* cur = base;
	if (cur == nullptr)
		cur = root;

	for (;;) {
		while (*path == '/')
			++path;

		if (*path == '\0')
			break;

		auto child = cur->get_child_node(path);
		if (is_fail(child))
			return null_pair(child.cause());

		cur = child.data();
		uptr len = nodename_length(path);
		path += len;
	}

	return make_pair(cause::OK, cur);
}


cause::t fs_ctl_init()
{
	fs_ctl* fsctl = new (generic_mem()) fs_ctl;
	if (!fsctl)
		return cause::NOMEM;

	auto r = fsctl->setup();
	if (is_fail(r)) {
		// TODO:logging return value
		new_destroy(fsctl, generic_mem());
		return r;
	}

	global_vars::core.fs_ctl_obj = fsctl;

	return cause::OK;
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

