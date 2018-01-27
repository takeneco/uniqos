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

/// Call from sys_mount.
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

/// Call from sys_unmount.
cause::t fs_ctl::unmount(
    process*    proc,
    const char* target,
    u64         flags)
{
    cause::pair<path_parser*> tgt_path = path_parser::create(proc, target);
    if (is_fail(tgt_path))
        return tgt_path.cause();

    fs_node* target_node = tgt_path->get_edge_fsnode();

    spin_wlock_section _sws(mountpoints_lock);

    fs_mount_info* target_mi = nullptr;
    for (fs_mount_info* mi : mountpoints) {
        if (target_node == mi->mount_obj->get_root_node()) {
            target_mi = mi;
            break;
        }
    }

    path_parser::destroy(tgt_path);

    if (!target_mi)
        return cause::BADARG;

    auto r = target_mi->mount_obj->get_driver()->unmount(
        target_mi->mount_obj, target_mi->target, flags);

    mountpoints_lock.wlock();
    mountpoints.remove(target_mi);
    mountpoints_lock.un_wlock();

    target_mi->mount_fsnode->remove_mount(target_mi);

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

fs_ctl* get_fs_ctl()
{
	return global_vars::core.fs_ctl_obj;
}


cause::t fs_ctl_setup()
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

