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

#include <core/log.hh>


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

/// @brief  Get child node instance.
//
/// Caller must call fs_node::refs.dec() of return value of this function
/// after use.
cause::pair<fs_node*> fs_dir_node::ref_child_node(const char* name)
{
    spin_wlock_section _wlocksec(child_nodes_lock);

    auto child = _search_cached_child_node(name);

    if (child.cause() == cause::NOENT)
        child = _ref_child_node(name);

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

/// @brief  Search fs_node from child_nodes.
cause::pair<fs_node*> fs_dir_node::search_cached_child_node(const char* name)
{
    spin_rlock_section _srs(child_nodes_lock);

    return _search_cached_child_node(name);
}

/// @brief  Search fs_node from child_nodes without lock.
cause::pair<fs_node*> fs_dir_node::_search_cached_child_node(const char* name)
{
    for (child_node* child : child_nodes) {
        if (fs::name_compare(child->name, name) == 0)
            return make_pair(cause::OK, child->node);
    }

    return null_pair(cause::NOENT);
}

/// @brief  Create fs_node instance corresponding to specified child node and
///         append to child_nodes.
/// @pre    specified child node is not exist in child_nodes.
cause::pair<fs_node*> fs_dir_node::_ref_child_node(
    const char* name)
{
    fs_mount* mnt = get_owner();

    auto child_fsn = mnt->acquire_node(this, name);
    if (is_fail(child_fsn))
        return child_fsn;

    child_node* child =
        new (mem_alloc(child_node::calc_size(name))) child_node;
    if (!child) {
        // TODO: destroy child_fsn
        cause::t c = mnt->release_node(child_fsn, this, name);
        if (is_fail(c))
            log()(SRCPOS)(": release_node() failed. r=").u(c);
        return null_pair(cause::NOMEM);
    }

    child->node = child_fsn.value();
    fs::name_copy(name, child->name);

    child_nodes.push_front(child);

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

