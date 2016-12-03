/// @file  device_ctl.cc
/// @brief device controller.

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include <core/device.hh>
#include <core/global_vars.hh>
#include <core/new_ops.hh>
#include <core/setup.hh>
#include <util/string.hh>


// device class

device::device(TYPE dev_type) :
	type(dev_type),
	dev_class(nullptr)
{
	name[0] = '\0';
}

void device::set_name(const char* device_name)
{
	str_copy(device_name, name, sizeof name - 1);
	name[sizeof name - 1] = '\0';
}

const char* device::get_name() const
{
	return name;
}

const char* device::get_class() const
{
	return dev_class;
}


// device_ctl class

device_ctl::device_ctl()
{
}

/// device::name で昇順に並ぶように追加する。
/// @retval cause::OK  Succeeded.
/// @retval cause::FAIL  Device name already exists.
cause::t device_ctl::append_device(device* dev)
{
	spin_wlock_section bcl(dev_chain_lock);

	device* insert_before = nullptr;

	for (device* cur_dev : dev_chain) {
		const sint comp = str_compare(
		    cur_dev->get_name(), dev->get_name(), device::NAME_NR);

		if (comp < 0)
			continue;
		else if (comp == 0)
			return cause::EXIST;
		else { //if (comp > 0)
			insert_before = cur_dev;
			break;
		}
	}

	if (insert_before)
		dev_chain.insert_before(insert_before, dev);
	else
		dev_chain.push_back(dev);

	return cause::OK;
}

cause::t device_ctl::remove_device(device* dev)
{
	dev_chain_lock.wlock();

	dev_chain.remove(dev);

	dev_chain_lock.un_wlock();

	return cause::OK;
}

locked_chain_iterator<device::iterative_chain, device>
device_ctl::each_devices()
{
	return locked_chain_iterator<device::iterative_chain, device>(
	    dev_chain.front(),
	    &dev_chain_lock);
}


// global functions

cause::t device_ctl_setup()
{
	device_ctl* devctl = new (generic_mem()) device_ctl;
	if (!devctl)
		return cause::NOMEM;

	global_vars::core.device_ctl_obj = devctl;

	return cause::OK;
}

device_ctl* get_device_ctl()
{
	return global_vars::core.device_ctl_obj;
}

