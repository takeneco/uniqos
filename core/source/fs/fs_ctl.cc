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

#include <core/fs.hh>
#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/mempool.hh>
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
	fs_dir_node* parent = get_parent_node(root, path);

	auto target_node = parent->create_child_reg_node(path);
	if (is_fail(target_node))
		return null_pair(target_node.cause());

	return target_node.data()->open(flags);
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

cause::t fs_ctl::mkdir(const char* path)
{
	fs_node* _parent = get_parent_node(root, path);
	if (!_parent->is_dir()) {
		return cause::NOTDIR;
	}
	fs_dir_node* parent = static_cast<fs_dir_node*>(_parent);

	// TODO:make dir
	fs_mount* mnt = parent->get_owner();

	const char* name = fs::nodename_get_last(path);

	return mnt->create_dir_node(parent, name, fs::OP_CREATE);
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

/// 指定したパスの親ディレクトリの fs_dir_node を返す。
/// @TODO:相対パスを考慮する。
cause::pair<fs_dir_node*> fs_ctl::get_parent_node(
    fs_dir_node* base, const char* path)
{
	fs_dir_node* cur = (*path == '/' ? root : base);

	while (*path) {
		fs_node* cur2 = cur->get_mounted_node();
		if (cur2 && cur2->is_dir())
			cur = static_cast<fs_dir_node*>(cur2);

		while (*path == '/')
			++path;

		if (fs::nodename_is_last(path))
			break;

		auto child = cur->get_child_node(path);
		if (is_fail(child))
			return null_pair(child.cause());

		if (!child.value()->is_dir())
			return null_pair(cause::NOENT);

		cur = static_cast<fs_dir_node*>(child.value());

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

