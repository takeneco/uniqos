/// @file  fs/fs_ctl.cc
/// @brief Filesystem controller.

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

#include <core/fs_ctl.hh>

#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/process.hh>
#include <core/setup.hh>
#include <util/string.hh>


using namespace fs;

// fs_ctl

fs_ctl::fs_ctl() :
	rootfs_mnt(&rootfs_drv)
{
}

cause::t fs_ctl::setup()
{
	fs_node* _root = rootfs_mnt.get_root_node();

	if (!_root->is_dir())
		return cause::FAIL;

	root = static_cast<fs_dir_node*>(_root);

	return cause::OK;
}

/// @brief ファイルシステムのドライバを検出する。
/// @param[in] source ファイルシステムの場所を示す文字列。
/// @param[in] type   ファイルシステム名。
/// @return ファイルシステムのドライバが見つかれば cause::OK を返す。
///         ドライバが見つからない場合は cause::NODEV を返す。
//
/// type をそのままドライバ名と解釈してファイルシステムドライバを探す。
/// type が nullptr の場合はファイルシステムのドライバへ source の文字列を
/// を渡してマウント可能か検証させ、マウントできるドライバを探す。
/// Caller must call fs_driver::refs.dec() after use.
cause::pair<fs_driver*> fs_ctl::detect_fs_driver(
    const char* source,
    const char* type)
{
	if (type) {
		auto drv = get_driver_ctl()->get_driver_by_name(type);
		if (is_ok(drv)) {
			fs_driver* fsdrv = static_cast<fs_driver*>(drv.value());
			if (drv->get_type() == driver::TYPE_FS)
				return make_pair(cause::OK, fsdrv);
			else
				fsdrv->refs.dec();
		}
	} else {
		for (driver* drv :
		     get_driver_ctl()->filter_by_type(driver::TYPE_FS))
		{
			fs_driver* fsdrv = static_cast<fs_driver*>(drv);
			if (fsdrv->mountable(source)) {
				fsdrv->refs.inc();
				return make_pair(cause::OK, fsdrv);
			}
		}
	}

	return null_pair(cause::NODEV);
}

cause::pair<io_node*> fs_ctl::open_node(
    generic_ns* fsns, const char* cwd, const char* path, u32 flags)
{
	cause::pair<path_parser*> pathnodes =
	    path_parser::create(fsns, cwd, path);
	if (is_fail(pathnodes))
		return null_pair(pathnodes.cause());

	fs_node* edge_node = pathnodes->get_edge_fsnode();

	if (!edge_node) {
		// create new node
		fs_dir_node* parent = static_cast<fs_dir_node*>(
		    pathnodes->get_edge_parent_fsnode());
		cause::pair<fs_reg_node*> tmp =
		    parent->create_child_reg_node(pathnodes->get_edge_name());
		if (is_fail(tmp)) {
			path_parser::destroy(pathnodes);
			return null_pair(tmp.cause());
		}
		edge_node = tmp.value();
	}

	edge_node->refs.inc();
	path_parser::destroy(pathnodes);

	return edge_node->open(flags);
}

cause::pair<io_node*> fs_ctl::open_node(
    process* proc, const char* path, u32 flags)
{
	spin_rlock_section ns_lock_sec(proc->ref_ns_lock());
	spin_rlock_section cwd_lock_sec(proc->ref_cwd_lock());

	generic_ns* proc_ns = proc->get_ns_fs();
	const char* proc_cwd = proc->get_cwd_path();

	return open_node(proc_ns, proc_cwd, path, flags);
}

cause::t fs_ctl::mount(
    process*    proc,   ///< mounter process.
    const char* source, ///< source device.
    const char* target, ///< mount target path.
    const char* type,   ///< fs type name.
    u64         flags,  ///< mount flags.
    const void* data)   ///< optional data.
{
	cause::pair<fs_driver*> fs_drv = detect_fs_driver(source, type);
	if (is_fail(fs_drv))
		return fs_drv.cause();

	cause::pair<path_parser*> tgt_path = path_parser::create(proc, target);
	if (is_fail(tgt_path)) {
		fs_drv->refs.dec();
		return tgt_path.cause();
	}

	fs_node* tgt_fsnode = tgt_path->get_edge_fsnode();

	cause::t r = _mount(source, target, tgt_fsnode, fs_drv, flags, data);

	path_parser::destroy(tgt_path);
	fs_drv->refs.dec();

	return r;
}

cause::t fs_ctl::unmount(
    process*    proc,
    const char* target,
    u64         flags)
{
	cause::pair<path_parser*> tgt_path = path_parser::create(proc, target);
	if (is_fail(tgt_path))
		return tgt_path.cause();

	spin_wlock_section _sws(mountpoints_lock);

	fs_mount_info* target_mi = nullptr;
	for (fs_mount_info* mi : mountpoints) {
		if (str_compare(tgt_path->get_path(), mi->target) == 0) {
			target_mi = mi;
			break;
		}
	}

	path_parser::destroy(tgt_path);

	if (!target_mi)
		return cause::BADARG;

	auto target_node = get_fs_node(nullptr, target, 0);
	if (is_fail(target_node))
		return target_node.cause();

	mountpoints.remove(target_mi);

	target_node.value()->remove_mount(target_mi);

	auto r = target_mi->mount_obj->get_driver()->unmount(
	    target_mi->mount_obj, target_mi->target, flags);

	fs_mount_info::destroy(target_mi);

	return r;
}

cause::t fs_ctl::mkdir(process* proc, const char* path)
{
	cause::pair<path_parser*> _path = path_parser::create(proc, path);
	if (is_fail(_path))
		return _path.cause();

	if (_path->get_edge_fsnode() != nullptr) {
		path_parser::destroy(_path.value());
		return cause::EXIST;
	}

	fs_node* _parent = _path->get_edge_parent_fsnode();
	if (!_parent->is_dir()) {
		path_parser::destroy(_path.value());
		return cause::NOTDIR;
	}

	fs_mount* mnt = _parent->get_owner();

	cause::t r = mnt->create_dir_node(
	    static_cast<fs_dir_node*>(_parent),
	    _path->get_edge_name(),
	    fs::OP_CREATE);

	path_parser::destroy(_path);

	return r;
}

cause::t fs_ctl::_mount(
    const char* source,
    const char* target,
    fs_node*    target_fsnode,
    fs_driver*  drv,
    u64         flags,
    const void* data)
{
	cause::pair<fs_mount_info*> mi = fs_mount_info::create(source, target);
	if (is_fail(mi))
		return mi.cause();

	fs_mount_info* mnt_info = mi.value();

	cause::pair<fs_mount*> mnt_obj = drv->mount(source);
	if (is_ok(mnt_obj)) {
		mnt_info->mount_obj = mnt_obj;
		mnt_info->mount_fsnode = target_fsnode;
		target_fsnode->append_mount(mnt_info);

		mountpoints_lock.wlock();
		mountpoints.push_front(mnt_info);
		mountpoints_lock.un_wlock();

		return cause::OK;
	}

	cause::t r2 = fs_mount_info::destroy(mnt_info);
	if (is_fail(r2)) {
		log()(SRCPOS)
		     (": fs_mount_info::destroy() failed. r=")
		   .u(r2)();
	}

	return mnt_obj.cause();
}

cause::pair<fs_node*> fs_ctl::get_fs_node(
    fs_node* base, const char* path, u32 flags)
{
	fs_node* cur = (fs::path_is_absolute(path) ? root : base);

	for (;;) {
		if (*path == '/' && !cur->is_dir())
			return null_pair(cause::NOENT);

		while (*path == '/')
			++path;

		if (*path == '\0')
			break;

		if (!cur->is_dir())  // TODO:this check is redundancy.
			return null_pair(cause::NOENT);

		auto child =
		    static_cast<fs_dir_node*>(cur)->get_child_node(path);
		if (is_fail(child))
			return null_pair(child.cause());

		cur = child.value();

		uptr len = fs::name_length(path);
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

