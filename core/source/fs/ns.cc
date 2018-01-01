/// @file  fs.cc
/// @brief Filesystem utils.

//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
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

#include <core/fs.hh>

#include <core/fs_ctl.hh>  // pathname_cn_t
#include <core/new_ops.hh>
#include <util/string.hh>


namespace fs {

cause::pair<fs_ns*> _create_initial_ns()
{
	const char INITIAL_DRV[] = "ramfs";
	const char INITIAL_SRC[] = "none";

	fs_ns* ns = new (generic_mem()) fs_ns();
	if (!ns)
		return null_pair(cause::NOMEM);

	auto mnt_info = fs_mount_info::create(INITIAL_SRC, "/");
	if (is_fail(mnt_info))
		return make_pair(mnt_info.cause(), ns);

	ns->add_mount_info(mnt_info.value());

	auto drv = get_driver_ctl()->get_driver_by_name(INITIAL_DRV);
	if (is_fail(drv))
		return make_pair(drv.cause(), ns);

	fs_driver* fs_drv = static_cast<fs_driver*>(drv.value());

	auto mnt_obj = fs_drv->mount(INITIAL_SRC);
	if (is_fail(mnt_obj))
		return make_pair(mnt_obj.cause(), ns);

	mnt_info.value()->mount_obj = mnt_obj.value();

	auto r = ns->set_root_mount_info(mnt_info.value());
	if (is_fail(r))
		return make_pair(r, ns);

	return make_pair(cause::OK, ns);
}

cause::t _destroy_ns(fs_ns* ns)
{
	// TODO: unmount all

	new_destroy(ns, generic_mem());
}

cause::pair<generic_ns*> create_initial_ns()
{
	cause::pair<fs_ns*> fsns = _create_initial_ns();
	if (is_fail(fsns)) {
		if (fsns.value())
			_destroy_ns(fsns.value());

		return null_pair(fsns.cause());
	}

	return cause::pair<generic_ns*>(fsns.cause(), fsns.value());
}

cause::t destroy_ns(generic_ns* ns)
{
	if (!ns || !ns->is_type(generic_ns::TYPE_FS))
		return cause::BADARG;

	return _destroy_ns(static_cast<fs_ns*>(ns));
}

}  // namespace fs

