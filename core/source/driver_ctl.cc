/// @file  driver_ctl.cc
/// @brief driver controller.

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

#include <core/driver.hh>

#include <core/global_vars.hh>
#include <core/new_ops.hh>
#include <core/setup.hh>
#include <util/string.hh>


// driver

driver::driver(const char* driver_name)
{
	str_copy(driver_name, name, sizeof name - 1);
	name[sizeof name - 1] = '\0';
}

driver::~driver()
{
}


// driver_ctl

driver_ctl::driver_ctl()
{
}

/// @retval cause::OK Succeeded.
/// @retval cause::FAIL Same name driver already exits.
cause::t driver_ctl::append_driver(driver* drv)
{
	spin_lock_section dll(driver_chain_lock);

	driver* insert_before = nullptr;

	for (driver* cur_drv : driver_chain) {
		const sint comp = str_compare(
		    cur_drv->get_name(), drv->get_name(), driver::NAME_NR);

		if (comp < 0)
			continue;
		else if (comp == 0)
			return cause::FAIL;

		insert_before = cur_drv;
		break;
	}

	if (insert_before)
		driver_chain.insert_before(insert_before, drv);
	else
		driver_chain.push_back(drv);

	return cause::OK;
}

cause::t driver_ctl::remove_driver(driver* drv)
{
	driver_chain_lock.lock();

	driver_chain.remove(drv);

	driver_chain_lock.unlock();

	return cause::OK;
}

/// @brief Search driver by name.
/// @retval cause::OK   Driver found. cause::data() is matched driver.
/// @retval cause::FAIL Driver not found.
cause::pair<driver*> driver_ctl::search_driver(const char* name)
{
	spin_lock_section _sls(driver_chain_lock);

	for (driver* drv : driver_chain) {
		const sint comp = str_compare(
		    name, drv->get_name(), driver::NAME_NR);

		if (comp < 0)
			continue;
		else if (comp == 0)
			return make_pair(cause::OK, drv);
		else //if (com > 0)
			break;
	}

	return null_pair(cause::FAIL);
}


cause::t driver_ctl_setup()
{
	driver_ctl* drvctl = new (generic_mem()) driver_ctl;
	if (!drvctl)
		return cause::NOMEM;

	global_vars::core.driver_ctl_obj = drvctl;

	return cause::OK;
}

driver_ctl* get_driver_ctl()
{
	return global_vars::core.driver_ctl_obj;
}

