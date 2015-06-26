/// @file   core/fs_ctl.hh
/// @brief  filesystem interface declaration.
//
// (C) 2014 KATO Takeshi
//

#ifndef CORE_FS_CTL_HH_
#define CORE_FS_CTL_HH_

#include <core/io_node.hh>
#include <util/spinlock.hh>


class fs_driver;
class fs_mount;
class fs_node;
class io_node;

typedef u16 pathname_cn_t;  ///< pathname char count type.
const pathname_cn_t PATHNAME_MAX = 0xffff;
const pathname_cn_t NAME_MAX = 4096;


class fs_driver
{
	DISALLOW_COPY_AND_ASSIGN(fs_driver);

public:
	struct interfaces
	{
		void init();

		typedef cause::pair<fs_mount*> (*MountIF)(
		    fs_driver* x, const char* dev);
		MountIF Mount;

		typedef cause::t (*UnmountIF)(
		    fs_driver* x, fs_mount* mount, const char* target,
		    u64 flags);
		UnmountIF Unmount;
	};

	// mount
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
	cause::pair<fs_mount*> mount(const char* dev) {
		return ifs->Mount(this, dev);
	}
	cause::t unmount(fs_mount* mount, const char* target, u64 flags) {
		return ifs->Unmount(this, mount, target, flags);
	}

protected:
	fs_driver() {}
	fs_driver(interfaces* _ifs) : ifs(_ifs) {}

protected:
	const interfaces* ifs;
};


class fs_mount
{
	DISALLOW_COPY_AND_ASSIGN(fs_mount);

	friend class fs_node;

public:

	chain_node<fs_mount>& chain_hook() { return _chain_node; }

	fs_node* get_root_node() { return root; }

public:
	struct interfaces
	{
		void init();

		typedef cause::pair<fs_node*> (*CreateNodeIF)(
		    fs_mount* x, fs_node* parent, const char* name, u32 flags);
		CreateNodeIF CreateNode;

		typedef cause::pair<fs_node*> (*GetChildNodeIF)(
		    fs_mount* x, fs_node* parent, const char* childname);
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
	    fs_mount* x, fs_node* parent, const char* name, u32 flags) {
		return static_cast<T*>(x)->
		    on_CreateNode(parent, name, flags);
	}
	static cause::pair<fs_node*> nofunc_CreateNode(
	    fs_mount*, fs_node*, const char*, u32) {
		return null_pair(cause::NOFUNC);
	}

	// GetChildNode
	template <class T>
	static cause::pair<fs_node*> call_on_GetChildNode(
	    fs_mount* x, fs_node* parent, const char* childname) {
		return static_cast<T*>(x)->
		    on_GetChildNode(parent, childname);
	}
	static cause::pair<fs_node*> nofunc_GetChildNode(
	    fs_mount*, fs_node*, const char*) {
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

	fs_driver* get_driver() { return driver; }

private:
	cause::pair<fs_node*> create_node(
	    fs_node* parent, const char* name, u32 flags) {
		return ifs->CreateNode(this, parent, name, flags);
	}

protected:
	fs_mount(fs_driver* drv, const interfaces* _ifs) :
		ifs(_ifs), driver(drv)
	{}

private:
	chain_node<fs_mount> _chain_node;

protected:
	const interfaces* ifs;
	fs_driver* driver;
	fs_node* root;
};

class mount_info
{
	DISALLOW_COPY_AND_ASSIGN(mount_info);

private:
	mount_info(const char* src, const char* tgt);

public:
	static cause::pair<mount_info*> create(
	    const char* source, const char* target);
	static cause::t destroy(mount_info* mp);

	chain_node<mount_info>& fs_ctl_chainnode() {
		return _fs_ctl_chainnode;
	}
	chain_node<mount_info>& fs_node_chainnode() {
		return _fs_node_chainnode;
	}

	fs_mount*   mount_obj;
	const char* source;
	const char* target;

	pathname_cn_t source_char_cn;  ///< source char count.
	pathname_cn_t target_char_cn;  ///< target char count.

private:
	chain_node<mount_info> _fs_ctl_chainnode;
	chain_node<mount_info> _fs_node_chainnode;

	char buf[0];
};

class fs_node
{
	DISALLOW_COPY_AND_ASSIGN(fs_node);

	class child_node
	{
	public:
		child_node() {}

		forward_chain_node<child_node> _child_chain_node;
		forward_chain_node<child_node>& child_chain_hook() {
			return _child_chain_node;
		}

		static uptr calc_size(const char* name);

		fs_node* node;
		char name[0];
	};

public:
	fs_node(fs_mount* owner, u32 _mode);

	fs_mount* get_owner() { return owner; }
	bool is_dir() const;
	bool is_regular() const;

	void insert_mount(mount_info* mi);
	void remove_mount(mount_info* mi);

	cause::pair<io_node*> open(u32 flags) {
		return owner->open_node(this, flags);
	}

	fs_node* get_mounted_node();
	cause::pair<fs_node*> get_child_node(const char* name, u32 flags);

private:
	cause::pair<fs_node*> search_child_node(const char* name);
	cause::pair<fs_node*> create_child_node(const char* name, u32 flags);

private:
	fs_mount* owner;
	u32 mode;

	front_fchain<mount_info, &mount_info::fs_node_chainnode> mounts;
	front_forward_fchain<child_node, &child_node::child_chain_hook>
	    child_nodes;

	spin_rwlock mounts_lock;
	spin_rwlock child_nodes_lock;
};


class fs_rootfs_drv : public fs_driver
{
	DISALLOW_COPY_AND_ASSIGN(fs_rootfs_drv);

public:
	fs_rootfs_drv();

private:
	fs_driver::interfaces fs_driver_ops;
};

class fs_rootfs_mnt : public fs_mount
{
	DISALLOW_COPY_AND_ASSIGN(fs_rootfs_mnt);

public:
	fs_rootfs_mnt(fs_rootfs_drv* drv);

private:
	fs_mount::interfaces fs_mount_ops;

	fs_node root_node;
};

class fs_ctl
{
	DISALLOW_COPY_AND_ASSIGN(fs_ctl);

	friend class fs_node;

public:
	enum NODE_MODE {
		NODE_UNKNOWN,
		NODE_DIR,
		NODE_REG,
	};
	enum OPEN_FLAGS {
		OPEN_CREATE   = 0x01,
		OPEN_WRITE    = 0x02,
	};

public:
	fs_ctl();

public:
	cause::t setup();

	cause::t register_ramfs_driver(fs_driver* drv) {
		ramfs_driver = drv;
		return cause::OK;
	}

	cause::pair<io_node*> open(const char* path, u32 flags);
	cause::t close(io_node* ion);
	cause::t mount(
	    const char* source, const char* target, const char* type);
	cause::t unmount(
	    const char* target, u64 flags);

private:
	cause::pair<fs_node*> get_fs_node(
	    fs_node* base, const char* path, u32 flags);

private:
	fs_driver* ramfs_driver;
	/// mount point list
	fchain<mount_info, &mount_info::fs_ctl_chainnode> mountpoints;

	spin_rwlock mountpoints_lock;

	fs_node* root;

	fs_rootfs_drv rootfs_drv;
	fs_rootfs_mnt rootfs_mnt;
};


fs_ctl* get_fs_ctl();


#endif  // include guard

