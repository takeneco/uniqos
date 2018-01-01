/// @file   core/fs_ctl.hh
/// @brief  filesystem interface declaration.
/// Filesystem driver must include this headder file.

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

#ifndef CORE_FS_CTL_HH_
#define CORE_FS_CTL_HH_

#include <core/devnode.hh>
#include <core/driver.hh>
#include <core/fs.hh>
#include <core/ns.hh>
#include <core/refcnt.hh>
#include <core/spinlock.hh>


class fs_driver;
class fs_mount;
class fs_mount_info;
class fs_node;
class fs_dir_node;
class fs_reg_node;
class fs_dev_node;
class fs_ns;

class io_node;
class process;

typedef u16 pathname_cn_t;  ///< pathname char count type.
const pathname_cn_t PATHNAME_MAX = 0xffff;
const pathname_cn_t NAME_MAX = 4096;

namespace fs {

enum NODETYPE
{
	NODETYPE_UNKNOWN,
	NODETYPE_DIR,
	NODETYPE_REG,
	NODETYPE_DEV,
};

enum FLAGS
{
	OP_CREATE = 0x1,
};

};

/// @brief  Base class of filesystem drivers.
//
/// Filesystem driver must implement this interface.
class fs_driver : public driver
{
	NONCOPYABLE(fs_driver);

public:
	struct interfaces
	{
		void init();

		using DetectLabelIF = cause::pair<uptr> (*)(
		    fs_driver* x, io_node* dev);
		DetectLabelIF DetectLabel;

		typedef cause::t (*MountableIF)(
		    fs_driver* x, const char* dev);
		MountableIF Mountable;

		typedef cause::pair<fs_mount*> (*MountIF)(
		    fs_driver* x, const char* dev);
		MountIF Mount;

		typedef cause::t (*UnmountIF)(
		    fs_driver* x, fs_mount* mount, const char* target,
		    u64 flags);
		UnmountIF Unmount;
	};

	// DetectLabel
	template<class T> static cause::pair<uptr> call_on_DetectLabel(
	    fs_driver* x, io_node* dev) {
		return static_cast<T*>(x)->on_DetectLabel(dev);
	}
	static cause::pair<uptr> nofunc_DetectLabel(
	    fs_driver*, io_node* /*dev*/) {
		return zero_pair(cause::NOFUNC);
	}

	// Mountable
	template<class T> static cause::t call_on_Mountable(
	    fs_driver* x, const char* dev) {
		return static_cast<T*>(x)->on_Mountable(dev);
	}
	static cause::t nofunc_Mountable(
	    fs_driver*, const char* /*dev*/) {
		return cause::NOFUNC;
	}

	// Mount
	template<class T> static cause::pair<fs_mount*> call_on_Mount(
	    fs_driver* x, const char* dev) {
		return static_cast<T*>(x)->on_Mount(dev);
	}
	static cause::pair<fs_mount*> nofunc_Mount(
	    fs_driver* /*x*/, const char* /*dev*/) {
		return null_pair(cause::NOFUNC);
	}

	// Unmount
	template<class T> static cause::t call_on_Unmount(
	    fs_driver* x, fs_mount* mount, const char* target, u64 flags) {
		return static_cast<T*>(x)->on_Unmount(mount, target, flags);
	}
	static cause::t nofunc_Unmount(
	    fs_driver*, fs_mount*, const char*, u64) {
		return cause::NOFUNC;
	}

public:
	cause::t mountable(const char* dev) {
		return ifs->Mountable(this, dev);
	}
	cause::pair<fs_mount*> mount(const char* dev) {
		return ifs->Mount(this, dev);
	}
	cause::t unmount(fs_mount* mount, const char* target, u64 flags) {
		return ifs->Unmount(this, mount, target, flags);
	}

protected:
	fs_driver(interfaces* _ifs, const char* name);

public:
	chain_node<fs_driver> _fs_ctl_chain_node;

protected:
	const interfaces* ifs;
};


/// @brief  Base class of Mount points interface.
//
/// Filesystem driver must implement this interface.
class fs_mount
{
	NONCOPYABLE(fs_mount);

	friend class fs_node;

public:
	chain_node<fs_mount>& chain_hook() { return _chain_node; }

	fs_dir_node* get_root_node() { return root; }

public:
	struct interfaces
	{
		void init();

		typedef cause::pair<fs_node*> (*CreateNodeIF)(
		    fs_mount* x,
		    fs_dir_node* parent,
		    const char* name,
		    u32 flags);
		CreateNodeIF CreateNode;

		typedef cause::pair<fs_dir_node*> (*CreateDirNodeIF)(
		    fs_mount* x,
		    fs_dir_node* parent,
		    const char* name,
		    u32 flags);
		CreateDirNodeIF CreateDirNode;

		typedef cause::pair<fs_reg_node*> (*CreateRegNodeIF)(
		    fs_mount* x,
		    fs_dir_node* parent,
		    const char* name);
		CreateRegNodeIF CreateRegNode;

		typedef cause::pair<fs_dev_node*> (*CreateDevNodeIF)(
		    fs_mount* x,
		    fs_dir_node* parent,
		    const char* name,
		    devnode_no no,
		    u32 flags);
		CreateDevNodeIF CreateDevNode;

		typedef cause::t (*ReleaseNodeIF)(
		    fs_mount* x,
		    fs_node* release_node,
		    fs_dir_node* parent,
		    const char* name);
		ReleaseNodeIF ReleaseNode;

		typedef cause::pair<fs_node*> (*GetChildNodeIF)(
		    fs_mount* x, fs_dir_node* parent, const char* childname);
		GetChildNodeIF GetChildNode;

		typedef cause::pair<io_node*> (*OpenNodeIF)(
		    fs_mount* x, fs_node* node, u32 flags);
		OpenNodeIF OpenNode;

		typedef cause::t (*CloseNodeIF)(
		    fs_mount* x, io_node* ion);
		CloseNodeIF CloseNode;
	};

	// CreateNode
	template <class T>
	static cause::pair<fs_node*> call_on_CreateNode(
	    fs_mount* x, fs_dir_node* parent, const char* name, u32 flags) {
		return static_cast<T*>(x)->
		    on_CreateNode(parent, name, flags);
	}
	static cause::pair<fs_node*> nofunc_CreateNode(
	    fs_mount*, fs_dir_node*, const char*, u32) {
		return null_pair(cause::NOFUNC);
	}

	// CreateDirNode
	template <class T>
	static cause::pair<fs_dir_node*> call_on_CreateDirNode(
	    fs_mount* x,
	    fs_dir_node* parent, const char* name, u32 flags) {
		return static_cast<T*>(x)->
		    on_CreateDirNode(parent, name, flags);
	}
	static cause::pair<fs_dir_node*> nofunc_CreateDirNode(
	    fs_mount*, fs_dir_node*, const char*, u32) {
		return null_pair(cause::NOFUNC);
	}

	// CreateRegNode
	template <class T>
	static cause::pair<fs_reg_node*> call_on_CreateRegNode(
	    fs_mount* x,
	    fs_dir_node* parent, const char* name) {
		return static_cast<T*>(x)->
		    on_CreateRegNode(parent, name);
	}
	static cause::pair<fs_reg_node*> nofunc_CreateRegNode(
	    fs_mount*, fs_dir_node*, const char*) {
		return null_pair(cause::NOFUNC);
	}

	// CreateDevNode
	template <class T>
	static cause::pair<fs_dev_node*> call_on_CreateDevNode(
	    fs_mount* x,
	    fs_dir_node* parent, const char* name, devnode_no no, u32 flags) {
		return static_cast<T*>(x)->
		    on_CreateDevNode(parent, name, no, flags);
	}
	static cause::pair<fs_dev_node*> nofunc_CreateDevNode(
	    fs_mount*, fs_dir_node*, const char*, devnode_no, u32) {
		return null_pair(cause::NOFUNC);
	}

	// ReleaseNode
	template <class T>
	static cause::t call_on_ReleaseNode(
	    fs_mount* x,
	    fs_node* release_node, fs_dir_node* parent, const char* name) {
		return static_cast<T*>(x)->
		    on_ReleaseNode(release_node, parent, name);
	}
	static cause::t nofunc_ReleaseNode(
	    fs_mount*, fs_node*, fs_dir_node*, const char*) {
		return cause::NOFUNC;
	}

	// GetChildNode
	template <class T>
	static cause::pair<fs_node*> call_on_GetChildNode(
	    fs_mount* x, fs_dir_node* parent, const char* childname) {
		return static_cast<T*>(x)->
		    on_GetChildNode(parent, childname);
	}
	static cause::pair<fs_node*> nofunc_GetChildNode(
	    fs_mount*, fs_dir_node*, const char*) {
		return null_pair(cause::NOFUNC);
	}

	// OpenNode
	template <class T>
	static cause::pair<io_node*> call_on_OpenNode(
	    fs_mount* x, fs_node* node, u32 flags) {
		return static_cast<T*>(x)->
		    on_OpenNode(node, flags);
	}
	static cause::pair<io_node*> nofunc_OpenNode(
	    fs_mount*, fs_node*, u32) {
		return null_pair(cause::NOFUNC);
	}

	// CloseNode
	template <class T>
	static cause::t call_on_CloseNode(
	    fs_mount* x, io_node* ion) {
		return static_cast<T*>(x)->
		    on_CloseNode(ion);
	}
	static cause::t nofunc_CloseNode(
	    fs_mount*, io_node*) {
		return cause::NOFUNC;
	}

	cause::pair<io_node*> open_node(fs_node* node, u32 flags) {
		return ifs->OpenNode(this, node, flags);
	}
	cause::t create_dir_node(
	    fs_dir_node* parent, const char* name, u32 flags);
	cause::pair<fs_reg_node*> create_reg_node(
	    fs_dir_node* parent, const char* name);

	cause::pair<fs_node*> create_node(
	    fs_dir_node* parent, const char* name, u32 flags) {
		return ifs->CreateNode(this, parent, name, flags);
	}

	fs_driver* get_driver() { return driver; }

protected:
	fs_mount(fs_driver* drv, const interfaces* _ifs) :
		ifs(_ifs), driver(drv)
	{}

private:
	chain_node<fs_mount> _chain_node;

protected:
	const interfaces* ifs;
	fs_driver* driver;
	fs_dir_node* root;
};

class fs_mount_info
{
	NONCOPYABLE(fs_mount_info);

private:
	fs_mount_info(const char* src, const char* tgt);

public:
	static cause::pair<fs_mount_info*> create(
	    const char* source, const char* target);
	static cause::t destroy(fs_mount_info* mp);

	fs_ns*      ns;
	fs_mount*   mount_obj;
	fs_node*    mount_fsnode;
	const char* source;
	const char* target;

	pathname_cn_t source_char_cn;  ///< source char count.
	pathname_cn_t target_char_cn;  ///< target char count.

	chain_node<fs_mount_info> fs_ns_chain_node;
	chain_node<fs_mount_info> fs_node_chain_node;

	char buf[0];
};

/// @brief  Base class of file nodes interface.
class fs_node
{
	NONCOPYABLE(fs_node);

protected:
	fs_node(fs_mount* owner, u32 type);

public:
	fs_mount* get_owner() { return owner; }
	bool is_dir() const;
	bool is_regular() const;
	bool is_device() const;

	fs_node* into_ns(generic_ns* ns);
	fs_node* ref_into_ns(generic_ns* ns);
	void append_mount(fs_mount_info* mi);
	void remove_mount(fs_mount_info* mi);

	cause::pair<io_node*> open(u32 flags) {
		return owner->open_node(this, flags);
	}

	cause::pair<fs_node*> get_child_node(const char* name);
	cause::pair<fs_node*> ref_child_node(const char* name);
	fs_node* get_mounted_node();

	refcnt<> refs;

private:
	fs_mount* owner;
	u32 node_type;

	front_chain<fs_mount_info, &fs_mount_info::fs_node_chain_node>
	    mounts;

	spin_rwlock mounts_lock;
};

/// @brief  Directory node interface.
class fs_dir_node : public fs_node
{
	class child_node
	{
	public:
		child_node() {}

		static uptr calc_size(const char* name);

		chain_node<child_node> fs_node_chain_node;
		fs_node* node;
		char name[0];
	};

public:
	fs_dir_node(fs_mount* owner);

public:
	cause::t append_child_node(fs_node* child, const char* name);
	cause::pair<fs_node*> get_child_node(const char* name);
	cause::pair<fs_node*> ref_child_node(const char* name);
	cause::pair<fs_reg_node*> create_child_reg_node(const char* name);

private:
	cause::pair<fs_node*> create_child_node(const char* name, u32 flags);
	cause::pair<fs_node*> search_cached_child_node(const char* name);

private:
	front_chain<child_node, &child_node::fs_node_chain_node>
	    child_nodes;

	spin_rwlock child_nodes_lock;
};

/// @brief  Regular file node interface.
class fs_reg_node : public fs_node
{
public:
	fs_reg_node(fs_mount* owner);
};

/// @brief  Device file node interface.
class fs_dev_node : public fs_node
{
public:
	fs_dev_node(fs_mount* owner, devnode_no no);

public:

private:
	devnode_no node_no;
};

/// @brief  rootfs driver.
class fs_rootfs_drv : public fs_driver
{
	NONCOPYABLE(fs_rootfs_drv);

public:
	fs_rootfs_drv();

private:
	fs_driver::interfaces fs_driver_ops;
};

/// @brief  Mount point interface of rootfs.
class fs_rootfs_mnt : public fs_mount
{
public:
	fs_rootfs_mnt(fs_rootfs_drv* drv);

private:
	fs_mount::interfaces fs_mount_ops;

	fs_dir_node root_node;
};

/// @brief Namespace of filesystem.
class fs_ns : public generic_ns
{
public:
	fs_ns();

public:
	void     add_mount_info(fs_mount_info* mnt_info);
	cause::t set_root_mount_info(fs_mount_info* mnt_info);

public:
	fs_dir_node* get_root();
	fs_dir_node* ref_root();

private:
	/// mount point list
	chain<fs_mount_info, &fs_mount_info::fs_ns_chain_node> mountpoints;
	spin_rwlock mountpoints_lock;

	fs_dir_node* root;
public:
	chain_node<fs_ns> fs_ctl_chainnode;
};

/// @brief  Filesystem controller.
class fs_ctl
{
	NONCOPYABLE(fs_ctl);

	friend class fs_node;

public:
	enum NODE_MODE {
		NODE_UNKNOWN,
		NODE_DIR,
		NODE_REG,
		NODE_DEV,
	};
	enum OPEN_FLAGS {
		OPEN_CREATE   = 0x01,
		OPEN_WRITE    = 0x02,
	};

public:
	fs_ctl();

public:
	cause::t setup();

	cause::pair<fs_driver*> detect_fs_driver(
	    const char* source, const char* type);

	cause::pair<io_node*> open_node(generic_ns* fsns, const char* cwd,
	    const char* path, u32 flags);
	cause::pair<io_node*> open_node(
	    process* proc,
	    const char* path,
	    u32 flags);
	cause::t close(io_node* ion);
	cause::t mount(
	    process*    proc,
	    const char* source,
	    const char* target,
	    const char* type,
	    u64         flags,
            const void* data);
	cause::t unmount(
	    process*    proc,
	    const char* target,
	    u64         flags);
	cause::t mkdir(const char* path);

private:
	cause::t _mount(
	    const char* source,
	    const char* target,
	    fs_node*    target_fsnode,
	    fs_driver*  drv,
	    u64         flags,
	    const void* data);
	cause::pair<fs_node*> get_fs_node(
	    fs_node* base, const char* path, u32 flags);
	cause::pair<fs_dir_node*> get_parent_node(
	    fs_dir_node* base, const char* path);

private:
	chain<fs_ns, &fs_ns::fs_ctl_chainnode> ns_chain;

	/// mount point list
	chain<fs_mount_info, &fs_mount_info::fs_ns_chain_node> mountpoints;
	spin_rwlock mountpoints_lock;

	fs_dir_node* root;

	fs_rootfs_drv rootfs_drv;
	fs_rootfs_mnt rootfs_mnt;
};


fs_ctl* get_fs_ctl();


#endif  // CORE_FS_CTL_HH_

