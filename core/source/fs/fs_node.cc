/// @file  fs_node.cc
/// @brief fs_node class implements.

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


using namespace fs;

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

bool fs_node::is_reg() const
{
	return node_type == fs::NODETYPE_REG;
}

bool fs_node::is_dev() const
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
	fs::name_copy(name, cn->name);

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
	fs::name_copy(name, child->name);

	child_nodes_lock.wlock();
	child_nodes.push_front(child);
	child_nodes_lock.un_wlock();

	return child_fsn;
}

// fs_dir_node::child_node

uptr fs_dir_node::child_node::calc_size(const char* name)
{
	return sizeof (child_node) + fs::name_length(name) + 1;
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

