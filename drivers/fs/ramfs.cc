/// @file  drivers/fs/ramfs.cc
/// @brief ramfs driver.

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <core/fs_ctl.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>


namespace {

class ramfs_driver : public fs_driver
{
public:
	ramfs_driver();

	cause::t setup();
	cause::t teardown();

	cause::pair<fs_mount*> on_Mount(const char* dev);
	cause::t on_Unmount(fs_mount* mount, const char* target, u64 flags);

	const io_node::operations* ref_io_node_ops() { return &io_node_ops; }

	mempool* get_fs_node_mp() { return fs_node_mp; }
	mempool* get_io_node_mp() { return io_node_mp; }

private:
	operations self_ops;
	fs_mount::operations mount_ops;
	io_node::operations io_node_ops;
	mempool* fs_node_mp;
	mempool* io_node_mp;
};

/// mount point
class ramfs_mount : public fs_mount
{
public:
	ramfs_mount(ramfs_driver* drv) :
		fs_mount(drv)
	{}

	cause::t mount(const char* dev);

	cause::pair<fs_node*> on_CreateNode(
	    fs_node* parent, u32 mode, const char* name);
	cause::pair<io_node*> on_OpenNode(
	    fs_node* node, u32 flags);

private:
	ramfs_driver* get_driver() {
		return static_cast<ramfs_driver*>(fs_mount::get_driver());
	}
	cause::pair<io_node*> create_io_node();
};

/// file node
class ramfs_node : public fs_node
{
public:
	ramfs_node(fs_mount* owner, u32 mode) :
		fs_node(owner, mode)
	{}
};

/// opened file node
class ramfs_io_node : public io_node
{
public:
	ramfs_io_node(const operations* _ops) :
		io_node(_ops)
	{}

	cause::pair<dir_entry*> on_GetDirEntry(
	    uptr buf_bytes, dir_entry* buf);
};


// ramfs_driver

ramfs_driver::ramfs_driver() :
	fs_driver(&self_ops),
	fs_node_mp(nullptr),
	io_node_mp(nullptr)
{
	self_ops.init();

	self_ops.Mount      = fs_driver::call_on_Mount<ramfs_driver>;
	self_ops.Unmount    = fs_driver::call_on_Unmount<ramfs_driver>;

	mount_ops.init();

	mount_ops.OpenNode  = fs_mount::call_on_OpenNode<ramfs_mount>;

	io_node_ops.init();

	io_node_ops.GetDirEntry  = io_node::call_on_GetDirEntry<ramfs_io_node>;
}

cause::t ramfs_driver::setup()
{
	auto mp = mempool::create_exclusive(
	    sizeof (ramfs_node),
	    arch::page::INVALID,
	    mempool::ENTRUST);
	if (is_fail(mp))
		return mp.cause();

	fs_node_mp = mp.data();

	mp = mempool::create_exclusive(
	    sizeof (ramfs_io_node),
	    arch::page::INVALID,
	    mempool::ENTRUST);
	if (is_fail(mp))
		return mp.cause();

	io_node_mp = mp.data();

	return cause::OK;
}

cause::t ramfs_driver::teardown()
{
	// TODO:release fs_node_mp,ionode_mp
	return cause::NOFUNC;
}

cause::pair<fs_mount*> ramfs_driver::on_Mount(const char* dev)
{
	ramfs_mount* ramfs = new (generic_mem()) ramfs_mount(this);
	if (!ramfs)
		return null_pair(cause::NOMEM);

	auto r = ramfs->mount(dev);
	if (is_fail(r)) {
		new_destroy(ramfs, generic_mem());

		return null_pair(r);
	}

	return cause::pair<fs_mount*>(cause::OK, ramfs);
}

cause::t ramfs_driver::on_Unmount(
    fs_mount* mount, const char* target, u64 flags)
{
	return cause::NOFUNC;
}

// ramfs_mount

cause::t ramfs_mount::mount(const char*)
{
	root = new (*get_driver()->get_fs_node_mp())
	    ramfs_node(this, fs_ctl::NODE_DIR);
	if (!root)
		return cause::NOMEM;

	return cause::OK;
}

cause::pair<fs_node*> ramfs_mount::on_CreateNode(
    fs_node* parent, u32 mode, const char* name)
{
	return make_pair(cause::OK, (fs_node*)0);
}

cause::pair<io_node*> ramfs_mount::on_OpenNode(
    fs_node* node, u32 flags)
{
	return make_pair(cause::OK, (io_node*)0);
}

cause::pair<io_node*> ramfs_mount::create_io_node()
{
	ramfs_io_node* ion = new (*get_driver()->get_io_node_mp())
	    ramfs_io_node(get_driver()->ref_io_node_ops());
	if (!ion)
		return null_pair(cause::NOMEM);

	return cause::make_pair<io_node*>(cause::OK, ion);
}

// ramfs_node

// ramfs_io_node

cause::pair<dir_entry*> ramfs_io_node::on_GetDirEntry(
    uptr buf_bytes, dir_entry* buf)
{
	return null_pair(cause::OK);
}

}  // namespace


cause::t ramfs_init()
{
	ramfs_driver* ramfs_drv = new (generic_mem()) ramfs_driver;
	if (!ramfs_drv)
		return cause::NOMEM;

	auto r = ramfs_drv->setup();
	if (is_fail(r)) {
		//TODO:return code
		ramfs_drv->teardown();
		return r;
	}

	r = get_fs_ctl()->register_ramfs_driver(ramfs_drv);
	if (is_fail(r))
		new_destroy(ramfs_drv, generic_mem());

	return r;
}

