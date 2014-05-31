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
#include <new_ops.hh>


namespace {

class ramfs_driver : public fs_driver
{
public:
	ramfs_driver();

	cause::pair<fs_mount*> on_fs_driver_mount(const char* dev);

private:
	operations ops_obj;
};

class ramfs_mount : public fs_mount
{
public:
	ramfs_mount() {}

	cause::t mount(const char* dev);
};


// ramfs_driver

ramfs_driver::ramfs_driver() :
	fs_driver(&ops_obj)
{
	ops_obj.init();

	ops_obj.mount = call_on_fs_driver_mount<ramfs_driver>;
}

cause::pair<fs_mount*> ramfs_driver::on_fs_driver_mount(const char* dev)
{
	ramfs_mount* ramfs = new (mem_alloc(sizeof (ramfs_mount))) ramfs_mount;
	if (!ramfs)
		return null_pair(cause::NOMEM);

	auto r = ramfs->mount(dev);
	if (is_fail(r)) {
		ramfs->~ramfs_mount();
		operator delete (ramfs);
		mem_dealloc(ramfs);
		return null_pair(r);
	}

	return cause::pair<fs_mount*>(cause::OK, ramfs);
}

// ramfs_mount

cause::t ramfs_mount::mount(const char*)
{
	return cause::OK;
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

