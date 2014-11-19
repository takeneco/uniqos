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
#include <util/string.hh>


namespace {

class ramfs_node;
class ramfs_io_node;

class ramfs_driver : public fs_driver
{
public:
	ramfs_driver();

	cause::t setup();
	cause::t unsetup();

	cause::pair<fs_mount*> on_Mount(const char* dev);
	cause::t on_Unmount(fs_mount* mount, const char* target, u64 flags);

	const fs_mount::interfaces* get_fs_mount_ifs() { return &mount_ifs; }
	const io_node::interfaces* ref_io_node_ifs() { return &io_node_ifs; }

	mempool* get_fs_node_mp() { return fs_node_mp; }

private:
	interfaces self_ifs;
	fs_mount::interfaces mount_ifs;
	io_node::interfaces io_node_ifs;
	mempool* fs_node_mp;
};

/// mount point
class ramfs_mount : public fs_mount
{
public:
	ramfs_mount(ramfs_driver* drv);

	ramfs_driver* get_driver() {
		return static_cast<ramfs_driver*>(fs_mount::get_driver());
	}
	mempool* get_block_mp() { return block_mp; }

	cause::t mount(const char* dev);

	cause::pair<fs_node*> on_CreateNode(
	    fs_node* parent, const char* name, u32 flags);
	cause::pair<fs_node*> on_GetChildNode(
	    fs_node* parent, const char* childname);
	cause::pair<io_node*> on_OpenNode(
	    fs_node* node, u32 flags);
	cause::t on_CloseNode(
	    io_node* ion);

private:
	cause::pair<io_node*> create_io_node(ramfs_node* fsn);
	cause::t destroy_io_node(ramfs_io_node* ion);

private:
	mempool* block_mp;
};

/// opened file node
class ramfs_io_node : public io_node
{
public:
	ramfs_io_node(const interfaces* _ops, ramfs_node* _fsn);

	static cause::t on_Close(ramfs_io_node* x);
	cause::pair<uptr> on_Read(
	    io_node::offset off, void* data, uptr bytes);
	cause::pair<uptr> on_Write(
	    io_node::offset off, const void* data, uptr bytes);

	cause::pair<dir_entry*> on_GetDirEntry(
	    uptr buf_bytes, dir_entry* buf);

private:
	ramfs_node* fsnode;
};

/// file node
class ramfs_node : public fs_node
{
	friend class ramfs_io_node;

public:
	ramfs_node(ramfs_mount* owner, u32 flags);

	ramfs_mount* get_owner() {
		return static_cast<ramfs_mount*>(fs_node::get_owner());
	}
	ramfs_io_node* get_io_node() {
		return &ramfs_ion;
	}

	cause::t destroy();

	cause::pair<uptr> read(uptr off, void* data, uptr bytes);
	cause::pair<uptr> write(uptr off, const void* data, uptr bytes);

private:
	ramfs_io_node ramfs_ion;
	uptr size_bytes;
	u8* blocks[8];
};


// ramfs_driver

ramfs_driver::ramfs_driver() :
	fs_driver(&self_ifs),
	fs_node_mp(nullptr)
{
	self_ifs.init();

	self_ifs.Mount      = fs_driver::call_on_Mount<ramfs_driver>;
	self_ifs.Unmount    = fs_driver::call_on_Unmount<ramfs_driver>;

	mount_ifs.init();

	mount_ifs.CreateNode  = fs_mount::call_on_CreateNode<ramfs_mount>;
	mount_ifs.OpenNode    = fs_mount::call_on_OpenNode<ramfs_mount>;
	mount_ifs.CloseNode   = fs_mount::call_on_CloseNode<ramfs_mount>;

	io_node_ifs.init();

	io_node_ifs.Close        = io_node::call_on_Close<ramfs_io_node>;
	io_node_ifs.Read         = io_node::call_on_Read<ramfs_io_node>;
	io_node_ifs.Write        = io_node::call_on_Write<ramfs_io_node>;
	io_node_ifs.GetDirEntry  = io_node::call_on_GetDirEntry<ramfs_io_node>;
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

	return cause::OK;
}

cause::t ramfs_driver::unsetup()
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

ramfs_mount::ramfs_mount(ramfs_driver* drv) :
	fs_mount(drv, drv->get_fs_mount_ifs()),
	block_mp(nullptr)
{
}

cause::t ramfs_mount::mount(const char*)
{
	root = new (*get_driver()->get_fs_node_mp())
	           ramfs_node(this, fs_ctl::NODE_DIR);
	if (!root)
		return cause::NOMEM;

	auto mp = mempool::acquire_shared(8196);
	if (is_fail(mp))
		return mp.cause();

	block_mp = mp.data();

	return cause::OK;
}

cause::pair<fs_node*> ramfs_mount::on_CreateNode(
    fs_node* /*parent*/,
    const char* /*name*/,
    u32 flags)
{
	if ((flags & fs_ctl::OPEN_CREATE) == 0)
		return null_pair(cause::NOENT);

	fs_node* fsn = new (*get_driver()->get_fs_node_mp())
	                   ramfs_node(this, flags);
	if (!fsn)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, fsn);
}

cause::pair<fs_node*> ramfs_mount::on_GetChildNode(
    fs_node* parent,
    const char* childname)
{
}

cause::pair<io_node*> ramfs_mount::on_OpenNode(
    fs_node* fsn, u32 flags)
{
	ramfs_node* ramfsn = static_cast<ramfs_node*>(fsn);

	return cause::pair<io_node*>(cause::OK, ramfsn->get_io_node());
}

cause::t ramfs_mount::on_CloseNode(
    io_node* /*ion*/)
{
	return cause::OK;
}


// ramfs_io_node

ramfs_io_node::ramfs_io_node(const interfaces* _ifs, ramfs_node* _fsn) :
	io_node(_ifs),
	fsnode(_fsn)
{
}

cause::t ramfs_io_node::on_Close(ramfs_io_node* x)
{
	return cause::OK;
}

cause::pair<uptr> ramfs_io_node::on_Read(
    io_node::offset off, void* data, uptr bytes)
{
	return fsnode->read(off, data, bytes);
}

cause::pair<uptr> ramfs_io_node::on_Write(
    io_node::offset off, const void* data, uptr bytes)
{
	return fsnode->write(off, data, bytes);
}

cause::pair<dir_entry*> ramfs_io_node::on_GetDirEntry(
    uptr buf_bytes, dir_entry* buf)
{
	return null_pair(cause::OK);
}


// ramfs_node

ramfs_node::ramfs_node(ramfs_mount* owner, u32 flags) :
	fs_node(owner, flags),
	ramfs_ion(owner->get_driver()->ref_io_node_ifs(), this),
	size_bytes(0)
{
	for (int i = 0; i < num_of_array(blocks); ++i)
		blocks[i] = nullptr;
}

cause::t ramfs_node::destroy()
{
	mempool* block_mp = get_owner()->get_block_mp();

	for (int i = 0; i < num_of_array(blocks); ++i) {
		if (blocks[i]) {
			auto r = block_mp->release(blocks[i]);
		}
	}

	return cause::OK;
}

cause::pair<uptr> ramfs_node::read(uptr off, void* data, uptr bytes)
{
	mempool* block_mp = get_owner()->get_block_mp();
	const uptr block_size = block_mp->get_obj_size();

	const uptr start_blk = off / block_size;

	u8* _data = static_cast<u8*>(data);
	uptr read_bytes = 0;
	for (uptr blk = start_blk;
	     blk < (num_of_array(blocks) - 1) && bytes > 0;
	     ++blk)
	{
		uptr start = off - block_size * blk;
		if (start >= size_bytes)
			break;

		uptr size = min(bytes, block_size - start);
		if ((start + size) >= size_bytes)
			size = size_bytes - off;

		if (!blocks[blk]) {
			mem_fill(0, _data, size);
		} else {
			mem_copy(&blocks[blk][start], _data, size);

			off += size;
			_data += size;
			bytes -= size;
			read_bytes += size;
		}
	}

	return make_pair(cause::OK, read_bytes);
}

cause::pair<uptr> ramfs_node::write(uptr off, const void* data, uptr bytes)
{
	mempool* block_mp = get_owner()->get_block_mp();
	const uptr block_size = block_mp->get_obj_size();

	const uptr start_blk = off / block_size;

	const u8* _data = static_cast<const u8*>(data);
	uptr write_bytes = 0;
	for (uptr blk = start_blk;
	     blk < (num_of_array(blocks) - 1) && bytes > 0;
	     ++blk)
	{
		u8* block;
		if (!blocks[blk]) {
			auto b = block_mp->acquire();
			if (is_fail(b))
				return make_pair(cause::NOMEM, write_bytes);

			blocks[blk] = static_cast<u8*>(b.data());
			mem_fill(0, blocks[blk], block_size);
		}
		block = blocks[blk];

		uptr start = off - block_size * blk;
		uptr size = min(bytes, block_size - start);
		mem_copy(_data, &block[start], size);

		off += size;
		_data += size;
		bytes -= size;
		write_bytes += size;

		if (off > size_bytes)
			size_bytes = off;
	}

	return make_pair(cause::OK, write_bytes);
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
		ramfs_drv->unsetup();
		return r;
	}

	r = get_fs_ctl()->register_ramfs_driver(ramfs_drv);
	if (is_fail(r))
		new_destroy(ramfs_drv, generic_mem());

	return r;
}

