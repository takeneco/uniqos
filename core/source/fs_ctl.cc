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

fs_node::fs_node(fs_mount* mount_owner, u32 type) :
	owner(mount_owner),
	node_type(type)
{
}

bool fs_node::is_dir() const
{
	return node_type == fs::NODETYPE_DIR;
}

bool fs_node::is_regular() const
{
	return node_type == fs::NODETYPE_REG;
}

bool fs_node::is_device() const
{
	return  node_type == fs::NODETYPE_DEV;
}

fs_node* fs_node::into_ns(generic_ns* ns)
{
	for (auto minfo : mounts) {
		if (minfo->ns == ns)
			return minfo->mount_obj->get_root_node();
	}

	return this;
}

fs_node* fs_node::ref_into_ns(generic_ns* ns)
{
	for (auto minfo : mounts) {
		if (minfo->ns == ns) {
			fs_node* fsn = minfo->mount_obj->get_root_node();
			fsn->refs.inc();
			this->refs.dec();
			return fsn;
		}
	}

	return this;
}

void fs_node::append_mount(fs_mount_info* minfo)
{
	spin_wlock_section _sws(mounts_lock);

	mounts.push_front(minfo);
}

void fs_node::remove_mount(fs_mount_info* minfo)
{
	spin_wlock_section _sws(mounts_lock);

	mounts.remove(minfo);
}

cause::pair<fs_node*> fs_node::get_child_node(const char* name)
{
	if (is_dir())
		return static_cast<fs_dir_node*>(this)->get_child_node(name);
	else
		return null_pair(cause::NOTDIR);
}

cause::pair<fs_node*> fs_node::ref_child_node(const char* name)
{
	if (is_dir())
		return static_cast<fs_dir_node*>(this)->ref_child_node(name);
	else
		return null_pair(cause::NOTDIR);
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


// fs_dir_node

fs_dir_node::fs_dir_node(fs_mount* owner) :
	fs_node(owner, fs::NODETYPE_DIR)
{
}

cause::t fs_dir_node::append_child_node(fs_node* child, const char* name)
{
	child_node* cn =
	    new (mem_alloc(child_node::calc_size(name))) child_node;
	if (!child)
		return cause::NOMEM;

	cn->node = child;
	str_copy(name, cn->name);

	child_nodes_lock.wlock();
	child_nodes.push_front(cn);
	child_nodes_lock.un_wlock();

	return cause::OK;
}

// TODO:この関数は指定した名前のファイルがなければファイルを作成して返すが、
// ファイルは作成しない仕様に変更したい。
/// 新しい仕様：
/// 指定した名前のファイルがあれば、そのファイルの fs_node を返す。
/// 指定した名前のファイルがなければ、失敗する。
cause::pair<fs_node*> fs_dir_node::get_child_node(const char* name)
{
	// TODO:返した値が解放されないように、関数の外までロック範囲を広げる
	// 必要がある。

	auto child = search_cached_child_node(name);

	if (child.cause() == cause::NOENT)
		child = create_child_node(name, 0);

	return child;
}

cause::pair<fs_node*> fs_dir_node::ref_child_node(const char* name)
{
	auto child = search_cached_child_node(name);

	if (child.cause() == cause::NOENT)
		child = create_child_node(name, 0);

	if (is_ok(child))
		child->refs.inc();

	return child;
}

/// fs_reg_node を作成して子ノードにする。
/// まだ同じ名前のファイルがないことを呼び出し元が保証する必要がある。
cause::pair<fs_reg_node*> fs_dir_node::create_child_reg_node(const char* name)
{
	return get_owner()->create_reg_node(this, name);
}

/// child_nodes から fs_node を探す。
cause::pair<fs_node*> fs_dir_node::search_cached_child_node(const char* name)
{
	spin_rlock_section _srs(child_nodes_lock);

	child_node* child;
	for (child = child_nodes.front();
	     child;
	     child = child_nodes.next(child))
	{
		if (0 == fs::name_compare(child->name, name))
			return cause::make_pair(cause::OK, child->node);
	}

	return null_pair(cause::NOENT);
}

/// @brief create child_node instance and set its members.
/// 子ノード（ファイル）に対応する fs_node を作成する。
/// 子ノードが存在しなければ失敗する。
/// 子ノード自体を作成するわけではない。
/// この関数は作成した fs_node を child_nodes へ追加する。
/// 呼び出し元で、すでに子ノードに対応する fs_node が child_nodes に
/// 追加されていないことを保証しなければならない。
cause::pair<fs_node*> fs_dir_node::create_child_node(
    const char* name, u32 flags)
{
	auto child_fsn = get_owner()->create_node(this, name, flags);
	if (is_fail(child_fsn))
		return child_fsn;

	child_node* child =
	    new (mem_alloc(child_node::calc_size(name))) child_node;
	if (!child) {
		// TODO: destroy child_fsn
		return null_pair(cause::NOMEM);
	}

	child->node = child_fsn.value();
	str_copy(name, child->name);

	child_nodes_lock.wlock();
	child_nodes.push_front(child);
	child_nodes_lock.un_wlock();

	return child_fsn;
}

// fs_dir_node::child_node

uptr fs_dir_node::child_node::calc_size(const char* name)
{
	return sizeof (child_node) + str_length(name) + 1;
}

// fs_reg_node
fs_reg_node::fs_reg_node(fs_mount* owner) :
	fs_node(owner, fs::NODETYPE_REG)
{
}

// fs_dev_node

fs_dev_node::fs_dev_node(fs_mount* owner, devnode_no no) :
	fs_node(owner, fs::NODETYPE_DEV),
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


cause::t fs_mkdir(const char* path)
{
	return get_fs_ctl()->mkdir(path);
}

