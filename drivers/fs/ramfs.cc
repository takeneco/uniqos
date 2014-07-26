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
#include <mempool_ctl.hh>
#include <new_ops.hh>


namespace {

class ramfs_driver : public fs_driver
{
public:
	ramfs_driver();

	cause::pair<fs_mount*> on_Mount(const char* dev);

	const io_node::operations* ref_io_node_ops() { return &io_node_ops; }

private:
	operations self_ops;
	fs_mount::operations mount_ops;
	io_node::operations io_node_ops;
};

class ramfs_mount : public fs_mount
{
public:
	ramfs_mount(ramfs_driver* drv) :
		driver(drv)
	{}

	cause::t mount(const char* dev);

	cause::pair<fs_node*> on_CreateNode(
	    fs_node* parent, u32 mode, const char* name);
	cause::pair<io_node*> on_OpenNode(
	    fs_node* node, u32 flags);

private:
	cause::pair<io_node*> create_io_node();

private:
	ramfs_driver* driver;
	mempool* fs_node_mp;
	mempool* io_node_mp;
};

class ramfs_node : public fs_node
{
public:
	ramfs_node(fs_mount* owner, u32 mode) :
		fs_node(owner, mode)
	{}
};

class ramfs_io_node : public io_node
{
public:
	ramfs_io_node(const operations* _ops) :
		io_node(_ops)
	{}

	cause::pair<dir_entry*> on_io_node_GetDirEntry(
	    uptr buf_bytes, dir_entry* buf);
};


// ramfs_driver

ramfs_driver::ramfs_driver() :
	fs_driver(&self_ops)
{
	self_ops.init();

	self_ops.Mount      = fs_driver::call_on_Mount<ramfs_driver>;

	mount_ops.init();

	mount_ops.OpenNode  = fs_mount::call_on_OpenNode<ramfs_mount>;

	io_node_ops.init();

	io_node_ops.GetDirEntry  =
	    io_node::call_on_io_node_GetDirEntry<ramfs_io_node>;
}

cause::pair<fs_mount*> ramfs_driver::on_Mount(const char* dev)
{
	ramfs_mount* ramfs =
	    new (shared_mem()) ramfs_mount(this);
	if (!ramfs)
		return null_pair(cause::NOMEM);

	auto r = ramfs->mount(dev);
	if (is_fail(r)) {
		ramfs->~ramfs_mount();
		operator delete (ramfs, shared_mem());
		mem_dealloc(ramfs);
		return null_pair(r);
	}

	return cause::pair<fs_mount*>(cause::OK, ramfs);
}

// ramfs_mount

cause::t ramfs_mount::mount(const char*)
{
	auto r = get_mempool_ctl()->exclusived_mempool(
	    sizeof (ramfs_node),
	    arch::page::INVALID,
	    mempool_ctl::ENTRUST,
	    &fs_node_mp);
	if (is_fail(r))
		return r;

	r = get_mempool_ctl()->exclusived_mempool(
	    sizeof (ramfs_io_node),
	    arch::page::INVALID,
	    mempool_ctl::ENTRUST,
	    &io_node_mp);
	if (is_fail(r)) {
		// TODO:release fs_node_mp
		return r;
	}

	root = new (*fs_node_mp) ramfs_node(this, fs_ctl::NODE_DIR);

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
	ramfs_io_node* ion =
	    new (*io_node_mp) ramfs_io_node(driver->ref_io_node_ops());
	if (!ion)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, static_cast<io_node*>(ion));
}

// ramfs_node

// ramfs_io_node

cause::pair<dir_entry*> ramfs_io_node::on_io_node_GetDirEntry(
    uptr buf_bytes, dir_entry* buf)
{
	return null_pair(cause::OK);
}

}  // namespace


cause::t ramfs_init()
{
	ramfs_driver* ramfs_drv =
	    new (mem_alloc(sizeof (ramfs_driver))) ramfs_driver;
	if (!ramfs_drv)
		return cause::NOMEM;

	auto r = get_fs_ctl()->register_ramfs_driver(ramfs_drv);
	if (is_fail(r)) {
		ramfs_drv->~ramfs_driver();
		operator delete (ramfs_drv, (void*)nullptr);
		mem_dealloc(ramfs_drv);
	}

	return r;
}

