/// @file  drivers/fs/devfs.cc
/// @brief devfs driver.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
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

#include <core/fs_ctl.hh>
#include <util/string.hh>


namespace {

const char driver_name_devfs[] = "devfs";

class devfs_driver : public fs_driver
{
public:
	devfs_driver();

	cause::t setup();
	cause::t unsetup();

	cause::pair<fs_mount*> on_Mount(const char* dev);
	cause::t on_Unmount(fs_mount* mount, const char* target, u64 flags);

private:
	interfaces self_ifs;

	fs_mount* devfs_mnt;
};


// devfs_driver

devfs_driver::devfs_driver() :
	fs_driver(&self_ifs, driver_name_devfs),
	devfs_mnt(nullptr)
{
	self_ifs.init();

	self_ifs.Mount      = fs_driver::call_on_Mount<devfs_driver>;
	self_ifs.Unmount    = fs_driver::call_on_Unmount<devfs_driver>;
}

/// @pre ramfs_driver::setup() was completed, because devfs is alias to ramfs.
cause::t devfs_driver::setup()
{
	auto r1 = get_fs_ctl()->get_fs_driver("ramfs");
	if (is_fail(r1))
		return r1.cause();

	fs_driver* ramfs_drv = r1.data();

	auto r2 = ramfs_drv->mount(nullptr);
	if (is_fail(r2))
		return r2.cause();

	devfs_mnt = r2.data();

	return cause::OK;
}

cause::t devfs_driver::unsetup()
{
	return cause::NOFUNC;
}

cause::pair<fs_mount*> devfs_driver::on_Mount(const char* /*dev*/)
{
	return cause::pair<fs_mount*>(cause::OK, devfs_mnt);
}

cause::t devfs_driver::on_Unmount(
    fs_mount* /*mount*/, const char* /*target*/, u64 /*flags*/)
{
	return cause::NOFUNC;
}

}  // namespace


cause::t devfs_init()
{
	devfs_driver* devfs_drv = new (generic_mem()) devfs_driver;
	if (!devfs_drv)
		return cause::NOMEM;

	auto r = devfs_drv->setup();
	if (is_fail(r)) {
		//TODO:return code
		devfs_drv->unsetup();
		return r;
	}

	r = get_fs_ctl()->register_fs_driver(devfs_drv);
	if (is_fail(r))
		new_destroy(devfs_drv, generic_mem());

	return r;
}

