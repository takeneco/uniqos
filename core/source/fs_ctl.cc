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

}  // namespace

using namespace fs;

// fs_driver::interfaces

void fs_driver::interfaces::init()
{
	DetectLabel = fs_driver::nofunc_DetectLabel;
	Mountable   = fs_driver::nofunc_Mountable;
	Mount       = fs_driver::nofunc_Mount;
	Unmount     = fs_driver::nofunc_Unmount;
}


// fs_driver

fs_driver::fs_driver(interfaces* _ifs, const char* name) :
	driver(TYPE_FS, name),
	ifs(_ifs)
{
}

// fs_mount::interfaces

void fs_mount::interfaces::init()
{
	CreateNode     = fs_mount::nofunc_CreateNode;
	CreateDirNode  = fs_mount::nofunc_CreateDirNode;
	CreateRegNode  = fs_mount::nofunc_CreateRegNode;
	CreateDevNode  = fs_mount::nofunc_CreateDevNode;
	ReleaseNode    = fs_mount::nofunc_ReleaseNode;
	GetChildNode   = fs_mount::nofunc_GetChildNode;
	OpenNode       = fs_mount::nofunc_OpenNode;
	CloseNode      = fs_mount::nofunc_CloseNode;
}

cause::t fs_mount::create_dir_node(
    fs_dir_node* parent, const char* name, u32 flags)
{
	auto child = ifs->CreateDirNode(this, parent, name, flags);
	if (is_fail(child))
		return child.cause();

	cause::t r = parent->append_child_node(child.data(), name);
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::pair<fs_reg_node*> fs_mount::create_reg_node(
    fs_dir_node* parent,
    const char* name)
{
	auto child = ifs->CreateRegNode(this, parent, name);
	if (is_fail(child))
		return child;

	child->refs.inc();

	cause::t r = parent->append_child_node(child.value(), name);
	if (is_fail(r)) {
		child->refs.dec();
		cause::t r2 = ifs->ReleaseNode(this, child, parent, name);
		if (is_fail(r2)) {
			log()(SRCPOS)(": ReleaseNode() failed\n");
		}
		return null_pair(r);
	}

	return child;
}


// fs_mount_info

fs_mount_info::fs_mount_info(const char* src, const char* tgt) :
	mount_obj(nullptr)
{
	source_char_cn = str_length(src);
	target_char_cn = str_length(tgt);

	pathlen_t src_len = src ? source_char_cn + 1 : 0;
	pathlen_t tgt_len = tgt ? target_char_cn + 1 : 0;

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


// fs_rootfs_drv

fs_rootfs_drv::fs_rootfs_drv() :
	fs_driver(&fs_driver_ops, driver_name_rootfs)
{
	fs_driver_ops.init();
}


// fs_rootfs_mnt

fs_rootfs_mnt::fs_rootfs_mnt(fs_rootfs_drv* drv) :
	fs_mount(drv, &fs_mount_ops),
	root_node(this)
{
	fs_mount_ops.init();

	root = &root_node;
}


// fs_ns

fs_ns::fs_ns() :
	generic_ns(TYPE_FS)
{
}

void fs_ns::add_mount_info(fs_mount_info* mnt_info)
{
	mountpoints_lock.wlock();

	mountpoints.push_back(mnt_info);

	mountpoints_lock.un_wlock();

	mnt_info->ns = this;
}

/// @brief  Set root mount info.
//
/// @param[in] mnt_info  Mount info of root.
cause::t fs_ns::set_root_mount_info(fs_mount_info* root_mnt_info)
{
	if (root_mnt_info->ns != this)
		return cause::BADARG;

	root = root_mnt_info->mount_obj->get_root_node();

	return cause::OK;
}

fs_dir_node* fs_ns::get_root()
{
	return root;
}

fs_dir_node* fs_ns::ref_root()
{
	root->refs.inc();
	return root;
}


cause::t fs_mkdir(process* proc, const char* path)
{
	return get_fs_ctl()->mkdir(proc, path);
}

