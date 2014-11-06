/// @file   core/fs_ctl.hh
/// @brief  filesystem interface declaration.
//
// (C) 2014 KATO Takeshi
//

#ifndef CORE_FS_CTL_HH_
#define CORE_FS_CTL_HH_

#include <core/io_node.hh>


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
	struct operations
	{
		void init();

		typedef cause::pair<fs_mount*> (*MountOP)(
		    fs_driver* x, const char* dev);
		MountOP Mount;

		typedef cause::t (*UnmountOP)(
		    fs_driver* x, fs_mount* mount, const char* target,
		    u64 flags);
		UnmountOP Unmount;
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
		return ops->Mount(this, dev);
	}
	cause::t unmount(fs_mount* mount, const char* target, u64 flags) {
		return ops->Unmount(this, mount, target, flags);
	}

protected:
	fs_driver() {}
	fs_driver(operations* _ops) : ops(_ops) {}

protected:
	const operations* ops;
};


class fs_mount
{
	DISALLOW_COPY_AND_ASSIGN(fs_mount);

	friend class fs_node;

public:

	bichain_node<fs_mount>& chain_hook() { return _chain_node; }

	fs_node* get_root_node() { return root; }

public:
	struct operations
	{
		void init();

		typedef cause::pair<fs_node*> (*CreateNodeOP)(
		    fs_mount* x, fs_node* parent, const char* name, u32 flags);
		CreateNodeOP CreateNode;

		typedef cause::pair<fs_node*> (*GetChildNodeOP)(
		    fs_mount* x, fs_node* parent, const char* childname);
		GetChildNodeOP GetChildNode;

		typedef cause::pair<io_node*> (*OpenNodeOP)(
		    fs_mount* x, fs_node* node, u32 flags);
		OpenNodeOP OpenNode;

		typedef cause::t (*CloseNodeOP)(
		    fs_mount* x, io_node* ion);
		CloseNodeOP CloseNode;
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
		return ops->OpenNode(this, node, flags);
	}

	fs_driver* get_driver() { return driver; }

private:
	cause::pair<fs_node*> create_node(
	    fs_node* parent, const char* name, u32 flags) {
		return ops->CreateNode(this, parent, name, flags);
	}

protected:
	fs_mount(fs_driver* drv, const operations* _ops) :
		ops(_ops), driver(drv)
	{}

private:
	bichain_node<fs_mount> _chain_node;

protected:
	const operations* ops;
	fs_driver* driver;
	fs_node* root;
};

class mount_info
{
private:
	mount_info(const char* src, const char* tgt);

public:
	static cause::pair<mount_info*> create(
	    const char* source, const char* target);
	static cause::t destroy(mount_info* mp);

	bichain_node<mount_info>& fs_ctl_chainnode() {
		return _fs_ctl_chainnode;
	}
	bichain_node<mount_info>& fs_node_chainnode() {
		return _fs_node_chainnode;
	}

	fs_mount*   mount_obj;
	const char* source;
	const char* target;

	pathname_cn_t source_char_cn;  ///< source char count.
	pathname_cn_t target_char_cn;  ///< target char count.

private:
	bichain_node<mount_info> _fs_ctl_chainnode;
	bichain_node<mount_info> _fs_node_chainnode;

	char buf[0];
};

class fs_node
{
	DISALLOW_COPY_AND_ASSIGN(fs_node);

	class child_node
	{
	public:
		child_node() {}

		chain_node<child_node> _child_chain_node;
		chain_node<child_node>& child_chain_hook() {
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

	cause::pair<fs_node*> create_child_node(const char* name, u32 flags);
	cause::pair<io_node*> open(u32 flags) {
		return owner->open_node(this, flags);
	}

	fs_node* get_mounted_node();
	cause::pair<fs_node*> get_child_node(const char* name, u32 flags);
	cause::t append_child_node(const char* name, fs_node* fsn);

private:
	fs_mount* owner;
	u32 mode;

	bichain<mount_info, &mount_info::fs_node_chainnode> mounts;
	chain<child_node, &child_node::child_chain_hook> child_nodes;
};


class fs_rootfs_drv : public fs_driver
{
	DISALLOW_COPY_AND_ASSIGN(fs_rootfs_drv);

public:
	fs_rootfs_drv();

private:
	fs_driver::operations fs_driver_ops;
};

class fs_rootfs_mnt : public fs_mount
{
	DISALLOW_COPY_AND_ASSIGN(fs_rootfs_mnt);

public:
	fs_rootfs_mnt(fs_rootfs_drv* drv);

private:
	fs_mount::operations fs_mount_ops;

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
	bibochain<mount_info, &mount_info::fs_ctl_chainnode> mountpoints;
	fs_node* root;

	fs_rootfs_drv rootfs_drv;
	fs_rootfs_mnt rootfs_mnt;
};


fs_ctl* get_fs_ctl();


#endif  // include guard

