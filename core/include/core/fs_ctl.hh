/// @file   core/fs_ctl.hh
/// @brief  filesystem interface declaration.
//
// (C) 2014 KATO Takeshi
//

#ifndef CORE_FS_CTL_HH_
#define CORE_FS_CTL_HH_

#include <core/basic.hh>
#include <core/io_node.hh>


class fs_driver;
class fs_mount;
class fs_node;
class io_node;

typedef u16 pathname_cn_t;  ///< pathname char count type.
const pathname_cn_t PATHNAME_MAX = 0xffff;
const pathname_cn_t NAME_MAX = 4096;

class fs_ctl
{
	DISALLOW_COPY_AND_ASSIGN(fs_ctl);

	friend class fs_node;

private:
	class mountpoint
	{
	private:
		mountpoint(const char* src, const char* tgt);

	public:
		static cause::pair<mountpoint*> create(
		    const char* source, const char* target);
		static cause::t destroy(mountpoint* mp);

		fs_mount*   mount_obj;
		const char* source;
		const char* target;

		/// Enable if multi mount sources exist for same mount target.
		mountpoint* down_mountpoint;

		bichain_node<mountpoint> _chain_node;
		bichain_node<mountpoint>& chain_hook() { return _chain_node; }

		pathname_cn_t source_char_cn;  ///< source char count.
		pathname_cn_t target_char_cn;  ///< target char count.

		char buf[0];
	};

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
	cause::t init();

	cause::t register_ramfs_driver(fs_driver* drv) {
		ramfs_driver = drv;
		return cause::OK;
	}

	cause::pair<io_node*> open(const char* path, u32 flags);
	cause::t mount(const char* type,
	    const char* source, const char* target);

private:
	cause::pair<fs_node*> get_fs_node(
	    fs_node* base, const char* path, u32 flags);

private:
	fs_driver* ramfs_driver;
	/// mount point list
	bibochain<mountpoint, &mountpoint::chain_hook> mountpoints;
	fs_node* root;
};


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

public:
	cause::pair<fs_mount*> mount(const char* dev) {
		return ops->Mount(this, dev);
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
		    fs_mount* x, fs_node* parent, u32 mode, const char* name);
		CreateNodeOP CreateNode;

		typedef cause::pair<io_node*> (*OpenNodeOP)(
		    fs_mount* x, fs_node* node, u32 flags);
		OpenNodeOP OpenNode;
	};

	// CreateNode
	template<class T>
	static cause::pair<fs_node*> call_on_CreateNode(
	    fs_mount* x, fs_node* parent, u32 mode, const char* name) {
		return static_cast<T*>(x)->
		    on_CreateNode(parent, mode, name);
	}
	static cause::pair<fs_node*> nofunc_CreateNode(
	    fs_mount*, fs_node*, u32, const char*) {
		return null_pair(cause::NOFUNC);
	}

	// OpenNode
	template<class T>
	static cause::pair<io_node*> call_on_OpenNode(
	    fs_mount* x, fs_node* node, u32 flags) {
		return static_cast<T*>(x)->
		    on_OpenNode(node, flags);
	}
	static cause::pair<io_node*> nofunc_OpenNode(
	    fs_mount*, fs_node*, u32) {
		return null_pair(cause::NOFUNC);
	}

	cause::pair<io_node*> open_node(fs_node* node, u32 flags) {
		return ops->OpenNode(this, node, flags);
	}

private:
	cause::pair<fs_node*> create_node(
	    fs_node* parent, u32 mode, const char* name) {
		return ops->CreateNode(this, parent, mode, name);
	}

protected:
	fs_mount() {}

private:
	bichain_node<fs_mount> _chain_node;

protected:
	operations* ops;
	fs_node* root;
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

		fs_node* node;
		char name[0];
	};

public:
	fs_node(fs_mount* owner, u32 _mode);

	fs_mount* get_mount() { return owner; }
	bool is_dir() const { return mode == fs_ctl::NODE_DIR; }
	bool is_regular() const { return mode == fs_ctl::NODE_REG; }

	cause::pair<fs_node*> create_child_node(u32 mode, const char* name);
	cause::pair<io_node*> open(u32 flags) {
		return owner->open_node(this, flags);
	}

	cause::pair<fs_node*> get_child_node(const char* name);

private:
	fs_mount* owner;
	u32 mode;

	chain<child_node, &child_node::child_chain_hook> child_nodes;
};


fs_ctl* get_fs_ctl();


#endif  // include guard

