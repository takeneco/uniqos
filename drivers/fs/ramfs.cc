/// @file  drivers/fs/ramfs.cc
/// @brief ramfs driver.

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <core/fs_ctl.hh>
#include <util/string.hh>


namespace {

const char driver_name_ramfs[] = "ramfs";

class ramfs_reg_node;
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

	mempool* get_fs_reg_node_mp() { return fs_reg_node_mp; }
	mempool* get_fs_dir_node_mp() { return fs_dir_node_mp; }
	mempool* get_fs_dev_node_mp() { return fs_dev_node_mp; }

private:
	interfaces self_ifs;
	fs_mount::interfaces mount_ifs;
	io_node::interfaces io_node_ifs;

	mempool* fs_reg_node_mp;
	mempool* fs_dir_node_mp;
	mempool* fs_dev_node_mp;
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
	    fs_dir_node* parent, const char* name, u32 flags);
	cause::pair<fs_dir_node*> on_CreateDirNode(
	    fs_dir_node* parent, const char* name);
	cause::pair<fs_reg_node*> on_CreateRegNode(
	    fs_dir_node* parent, const char* name);
	cause::pair<fs_dev_node*> on_CreateDevNode(
	    fs_dir_node* parent, const char* name, devnode_no no, u32 flags);
	cause::pair<fs_node*> on_GetChildNode(
	    fs_dir_node* parent, const char* childname);
	cause::pair<io_node*> on_OpenNode(
	    fs_node* node, u32 flags);
	cause::t on_CloseNode(
	    io_node* ion);

private:
	cause::pair<io_node*> create_io_node(ramfs_reg_node* fsn);
	cause::t destroy_io_node(ramfs_io_node* ion);

private:
	mempool* block_mp;
};

/// opened file node
class ramfs_io_node : public io_node
{
public:
	ramfs_io_node(const interfaces* _ops, fs_node* _fsn);

public:
	static cause::t on_Close(ramfs_io_node* x);
	cause::pair<uptr> on_Read(
	    io_node::offset off, void* data, uptr bytes);
	cause::pair<uptr> on_Write(
	    io_node::offset off, const void* data, uptr bytes);

	cause::pair<dir_entry*> on_GetDirEntry(
	    uptr buf_bytes, dir_entry* buf);

private:
	fs_node* fsnode;
};

/// file node
class ramfs_reg_node : public fs_reg_node
{
	friend class ramfs_io_node;

public:
	ramfs_reg_node(ramfs_mount* owner);

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

/// directory node
class ramfs_dir_node : public fs_dir_node
{
public:
	ramfs_dir_node(ramfs_mount* owner);

	ramfs_mount* get_owner() {
		return static_cast<ramfs_mount*>(fs_node::get_owner());
	}
	ramfs_io_node* get_io_node() {
		return &ramfs_ion;
	}

private:
	ramfs_io_node ramfs_ion;
};


// ramfs_driver

ramfs_driver::ramfs_driver() :
	fs_driver(&self_ifs, driver_name_ramfs),
	fs_reg_node_mp(nullptr),
	fs_dir_node_mp(nullptr),
	fs_dev_node_mp(nullptr)
{
	self_ifs.init();

	self_ifs.Mount      = fs_driver::call_on_Mount<ramfs_driver>;
	self_ifs.Unmount    = fs_driver::call_on_Unmount<ramfs_driver>;

	mount_ifs.init();

	mount_ifs.CreateNode    = fs_mount::call_on_CreateNode<ramfs_mount>;
	mount_ifs.CreateDirNode = fs_mount::call_on_CreateDirNode<ramfs_mount>;
	mount_ifs.CreateDevNode = fs_mount::call_on_CreateDevNode<ramfs_mount>;
	mount_ifs.OpenNode      = fs_mount::call_on_OpenNode<ramfs_mount>;
	mount_ifs.CloseNode     = fs_mount::call_on_CloseNode<ramfs_mount>;

	io_node_ifs.init();

	io_node_ifs.Close        = io_node::call_on_Close<ramfs_io_node>;
	io_node_ifs.Read         = io_node::call_on_Read<ramfs_io_node>;
	io_node_ifs.Write        = io_node::call_on_Write<ramfs_io_node>;
	io_node_ifs.GetDirEntry  = io_node::call_on_GetDirEntry<ramfs_io_node>;
}

cause::t ramfs_driver::setup()
{
	auto mp = mempool::create_exclusive(
	    sizeof (ramfs_reg_node),
	    arch::page::INVALID,
	    mempool::ENTRUST);
	if (is_fail(mp))
		return mp.cause();

	fs_reg_node_mp = mp.data();

	mp = mempool::acquire_shared(sizeof (fs_dir_node));
	if (is_fail(mp))
		return mp.cause();

	fs_dir_node_mp = mp.data();

	mp = mempool::acquire_shared(sizeof (fs_dev_node));
	if (is_fail(mp))
		return mp.cause();

	fs_dev_node_mp = mp.data();

	return cause::OK;
}

cause::t ramfs_driver::unsetup()
{
	// TODO:release fs_reg_node_mp,ionode_mp
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
	root = new (*get_driver()->get_fs_dir_node_mp())
	           ramfs_dir_node(this);
	if (!root)
		return cause::NOMEM;

	auto mp = mempool::acquire_shared(8196);
	if (is_fail(mp))
		return mp.cause();

	block_mp = mp.data();

	return cause::OK;
}

/// この関数は存在しないファイルを指定された場合は失敗するべきだが、
/// ramfs の場合はすべてのファイルがキャッシュとして保存されているため
/// キャッシュを検索した結果をそのまま返せばよい。
/// しかし、呼び出し元はキャッシュを検索してファイルが見つからなかった
/// ときだけこの関数を呼び出すので、ramfsの場合のこの関数の動作は常に
/// 失敗するだけでもよい。
cause::pair<fs_node*> ramfs_mount::on_CreateNode(
    fs_dir_node* parent,
    const char* name,
    u32 /*flags*/)
{
	return parent->get_child_node(name);
}

cause::pair<fs_dir_node*> ramfs_mount::on_CreateDirNode(
    fs_dir_node* /*parent*/,
    const char* /*name*/)
{
	fs_dir_node* fsdn = new (*get_driver()->get_fs_dir_node_mp())
	                        ramfs_dir_node(this);
	if (!fsdn)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, fsdn);
}

cause::pair<fs_reg_node*> ramfs_mount::on_CreateRegNode(
    fs_dir_node* /*parent*/,
    const char* /*name*/)
{
	ramfs_reg_node* fsn = new (*get_driver()->get_fs_reg_node_mp())
	                          ramfs_reg_node(this);
	if (!fsn)
		return null_pair(cause::NOMEM);

	return cause::make_pair<fs_reg_node*>(cause::OK, fsn);
}

cause::pair<fs_dev_node*> ramfs_mount::on_CreateDevNode(
    fs_dir_node* parent,
    const char*,
    devnode_no no,
    u32 flags)
{
	fs_dev_node* node = new (*get_driver()->get_fs_dev_node_mp())
	                        fs_dev_node(this, no);
	if (!node)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, node);
}

cause::pair<fs_node*> ramfs_mount::on_GetChildNode(
    fs_dir_node* parent,
    const char* childname)
{
	return null_pair(cause::NOFUNC);
}

cause::pair<io_node*> ramfs_mount::on_OpenNode(
    fs_node* fsn, u32 flags)
{
	ramfs_reg_node* ramfsn = static_cast<ramfs_reg_node*>(fsn);

	return cause::pair<io_node*>(cause::OK, ramfsn->get_io_node());
}

cause::t ramfs_mount::on_CloseNode(
    io_node* /*ion*/)
{
	return cause::OK;
}


// ramfs_io_node

ramfs_io_node::ramfs_io_node(const interfaces* _ifs, fs_node* _fsn) :
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
	if (fsnode->is_regular()) {
		return static_cast<ramfs_reg_node*>(fsnode)->
		       read(off, data, bytes);
	} else {
		return zero_pair(cause::FAIL);
	}
}

cause::pair<uptr> ramfs_io_node::on_Write(
    io_node::offset off, const void* data, uptr bytes)
{
	if (fsnode->is_regular()) {
		return static_cast<ramfs_reg_node*>(fsnode)->
		       write(off, data, bytes);
	} else {
		return zero_pair(cause::FAIL);
	}
}

cause::pair<dir_entry*> ramfs_io_node::on_GetDirEntry(
    uptr buf_bytes, dir_entry* buf)
{
	return null_pair(cause::OK);
}


// ramfs_reg_node

ramfs_reg_node::ramfs_reg_node(ramfs_mount* owner) :
	fs_reg_node(owner),
	ramfs_ion(owner->get_driver()->ref_io_node_ifs(), this),
	size_bytes(0)
{
	for (uint i = 0; i < num_of_array(blocks); ++i)
		blocks[i] = nullptr;
}

cause::t ramfs_reg_node::destroy()
{
	mempool* block_mp = get_owner()->get_block_mp();

	for (uint i = 0; i < num_of_array(blocks); ++i) {
		if (blocks[i]) {
			auto r = block_mp->release(blocks[i]);
		}
	}

	return cause::OK;
}

cause::pair<uptr> ramfs_reg_node::read(uptr off, void* data, uptr bytes)
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

cause::pair<uptr> ramfs_reg_node::write(uptr off, const void* data, uptr bytes)
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

// ramfs_dir_node

ramfs_dir_node::ramfs_dir_node(ramfs_mount* owner) :
	fs_dir_node(owner),
	ramfs_ion(owner->get_driver()->ref_io_node_ifs(), this)
{
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

	r = get_fs_ctl()->register_fs_driver(ramfs_drv);
	if (is_fail(r))
		new_destroy(ramfs_drv, generic_mem());

	return r;
}

