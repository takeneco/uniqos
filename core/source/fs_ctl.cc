/// @file  fs_ctl.cc
/// @brief Filesystem controller.

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include <core/fs_ctl.hh>

#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/setup.hh>
#include <util/string.hh>


namespace {

const char* driver_name_rootfs = "rootfs";

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

bool nodename_is_not_dir(const char* path)
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

bool nodename_is_end(const char* path)
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

// fs_driver::interfaces

void fs_driver::interfaces::init()
{
	Mount    = fs_driver::nofunc_Mount;
	Unmount  = fs_driver::nofunc_Unmount;
}


// fs_driver

fs_driver::fs_driver(interfaces* _ifs, const char* name) :
	ifs(_ifs),
	fs_driver_name(name)
{
}

// fs_mount::interfaces

void fs_mount::interfaces::init()
{
	CreateNode     = fs_mount::nofunc_CreateNode;
	CreateDevNode  = fs_mount::nofunc_CreateDevNode;
	GetChildNode   = fs_mount::nofunc_GetChildNode;
	OpenNode       = fs_mount::nofunc_OpenNode;
	CloseNode      = fs_mount::nofunc_CloseNode;
}


// fs_mount_info

fs_mount_info::fs_mount_info(const char* src, const char* tgt) :
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

cause::pair<fs_mount_info*> fs_mount_info::create(
    const char* source, const char* target)
{
	uptr size = sizeof (fs_mount_info);
	size += source ? str_length(source) + 1 : 0;
	size += target ? str_length(target) + 1 : 0;

	fs_mount_info* mp = new (mem_alloc(size)) fs_mount_info(source, target);
	if (!mp)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, mp);
}

cause::t fs_mount_info::destroy(fs_mount_info* mp)
{
	mp->~fs_mount_info();
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

void fs_node::insert_mount(fs_mount_info* mi)
{
	spin_wlock_section _sws(mounts_lock);

	mounts.push_front(mi);
}

void fs_node::remove_mount(fs_mount_info* mi)
{
	spin_wlock_section _sws(mounts_lock);

	mounts.remove(mi);
}

fs_node* fs_node::get_mounted_node()
{
	// TODO:返した値が解放されないように、関数の外までロック範囲を広げる
	// 必要がある。
	spin_rlock_section _srs(mounts_lock);

	fs_mount_info* mnt = mounts.front();
	if (mnt)
		return mnt->mount_obj->get_root_node();

	return nullptr;
}

cause::pair<fs_node*> fs_node::get_child_node(const char* name, u32 flags)
{
	// TODO:返した値が解放されないように、関数の外までロック範囲を広げる
	// 必要がある。

	auto child = search_child_node(name);

	if (child.cause() == cause::NOENT &&
	   (flags & fs_ctl::OPEN_CREATE))
	{
		child = create_child_node(name, flags);
	}

	return child;
}

cause::pair<fs_node*> fs_node::search_child_node(const char* name)
{
	spin_rlock_section _srs(child_nodes_lock);

	child_node* child;
	for (child = child_nodes.front();
	     child;
	     child = child_nodes.next(child))
	{
		if (0 == nodename_compare(child->name, name))
			return cause::make_pair(cause::OK, child->node);
	}

	return null_pair(cause::NOENT);
}

/// @brief create child_node instance and set its members.
cause::pair<fs_node*> fs_node::create_child_node(const char* name, u32 flags)
{
	auto child_fsn = owner->create_node(this, name, flags);
	if (is_fail(child_fsn))
		return child_fsn;

	child_node* child =
	    new (mem_alloc(child_node::calc_size(name))) child_node;
	if (!child) {
		// TODO: destroy child_fsn
		return null_pair(cause::NOMEM);
	}

	child->node = child_fsn.data();
	str_copy(name, child->name);

	child_nodes_lock.wlock();
	child_nodes.push_front(child);
	child_nodes_lock.un_wlock();

	return child_fsn;
}


// fs_node::child_node

uptr fs_node::child_node::calc_size(const char* name)
{
	return sizeof (child_node) + str_length(name) + 1;
}


// fs_dir_node

fs_dir_node::fs_dir_node(fs_mount* owner) :
	fs_node(owner, fs_ctl::NODE_DIR)
{
}


// fs_dev_node

fs_dev_node::fs_dev_node(fs_mount* owner, devnode_no no) :
	fs_node(owner, fs_ctl::NODE_DEV),
	node_no(no)
{
}


// fs_rootfs_drv

fs_rootfs_drv::fs_rootfs_drv() :
	fs_driver(&fs_driver_ops, driver_name_rootfs)
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

cause::t fs_ctl::register_fs_driver(fs_driver* drv)
{
	auto r = get_fs_driver(drv->get_name());
	// 同名のドライバは登録禁止
	if (is_ok(r))
		return cause::FAIL;

	fs_driver_chain.push_front(drv);

	return cause::OK;
}

cause::pair<fs_driver*> fs_ctl::get_fs_driver(const char* name)
{
	for (auto drv : fs_driver_chain) {
		if (str_compare(name, drv->get_name()) == 0)
			return make_pair(cause::OK, drv);
	}

	return null_pair(cause::NODEV);
}

cause::pair<io_node*> fs_ctl::open(const char* path, u32 flags)
{
	fs_node* cur = root;

	while (*path) {
		fs_node* cur2 = cur->get_mounted_node();
		if (cur2)
			cur = cur2;

		while (*path == '/')
			++path;

		if (nodename_is_end(path))
			break;

		auto child = cur->get_child_node(path, 0);
		if (is_fail(child))
			return null_pair(child.cause());
		cur = child.data();

		uptr len = nodename_length(path);
		path += len;
	}

	auto target_node = cur->get_child_node(path, flags);
	if (is_fail(target_node))
		return null_pair(target_node.cause());

	return target_node.data()->open(flags);
}

cause::t fs_ctl::mount(const char* source, const char* target, const char* type)
{
	auto fs_drv = get_fs_driver(type);
	if (is_fail(fs_drv))
		return fs_drv.cause();

	auto target_node = get_fs_node(nullptr, target, 0);
	if (is_fail(target_node))
		return target_node.cause();

	cause::t r = cause::FAIL;

	auto r1 = fs_mount_info::create(source, target);
	if (is_ok(r1)) {
		fs_mount_info* mnt_info = r1.data();

		auto r2 = fs_drv.data()->mount(source);
		if (is_ok(r2)) {
			mnt_info->mount_obj = r2.data();

			target_node.data()->insert_mount(mnt_info);

			mountpoints_lock.wlock();
			mountpoints.push_front(mnt_info);
			mountpoints_lock.un_wlock();

			return cause::OK;
		}

		r = r2.cause();

		cause::t r3 = fs_mount_info::destroy(mnt_info);
		if (is_fail(r3)) {
			log()(SRCPOS)
			     (": fs_mount_info::destroy() failed. r=")
			   .u(r3)();
		}
	} else {
		r = r1.cause();
	}

	return r;
}

cause::t fs_ctl::unmount(const char* target, u64 flags)
{
	spin_wlock_section _sws(mountpoints_lock);
	
	fs_mount_info* target_mp = nullptr;
	for (fs_mount_info* mp = mountpoints.front();
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

	fs_mount_info::destroy(target_mp);

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

		auto child = cur->get_child_node(path, 0);
		if (is_fail(child))
			return null_pair(child.cause());

		cur = child.data();
		uptr len = nodename_length(path);
		path += len;
	}

	return make_pair(cause::OK, cur);
}

fs_ctl* get_fs_ctl()
{
	return global_vars::core.fs_ctl_obj;
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

