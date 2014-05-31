/// @file   core/fs_ctl.hh
/// @brief  filesystem interface declaration.
//
// (C) 2014 KATO Takeshi
//

#ifndef CORE_FS_CTL_HH_
#define CORE_FS_CTL_HH_

#include <core/basic.hh>


class fs_driver;
class fs_mount;
class fs_node;

class fs_ctl
{
	DISALLOW_COPY_AND_ASSIGN(fs_ctl);
private:
	class mountpoint
	{
	private:
		mountpoint(const char* src, const char* tgt);

	public:
		static cause::pair<mountpoint*> create(
		    const char* source, const char* target);

		fs_mount*   mount_obj;
		const char* source;
		const char* target;

		bichain_node<mountpoint> _chain_node;
		bichain_node<mountpoint>& chain_hook() { return _chain_node; }

		char buf[0];
	};

public:
	fs_ctl();

public:
	cause::t init();

	cause::t register_ramfs_driver(fs_driver* drv) {
		ramfs_driver = drv;
		return cause::OK;
	}

	cause::t mount(const char* type,
	    const char* source, const char* target);

private:
	fs_driver* ramfs_driver;
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

		typedef cause::pair<fs_mount*> (*mount_op)(
		    fs_driver* x, const char* dev);
		mount_op mount;
	};

	// mount
	template<class T> static cause::pair<fs_mount*> call_on_fs_driver_mount(
	    fs_driver* x, const char* dev) {
		return static_cast<T*>(x)->on_fs_driver_mount(dev);
	}
	static cause::pair<fs_mount*> nofunc_fs_driver_mount(
	    fs_driver* /*x*/, const char* /*dev*/) {
		return null_pair(cause::NOFUNC);
	}

public:
	cause::pair<fs_mount*> mount(const char* dev) {
		return ops->mount(this, dev);
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
public:
	fs_mount() {}

	bichain_node<fs_mount>& chain_hook() { return _chain_node; }

private:
	bichain_node<fs_mount> _chain_node;
};


class fs_node
{
	DISALLOW_COPY_AND_ASSIGN(fs_node);
public:
	fs_node() {}

};


fs_ctl* get_fs_ctl();


#endif  // include guard

