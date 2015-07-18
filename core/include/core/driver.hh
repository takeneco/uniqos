/// @file   core/driver_ctl.hh
/// @brief  driver management interface declaration.

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

#ifndef CORE_DRIVER_CTL_HH_
#define CORE_DRIVER_CTL_HH_

#include <core/basic.hh>
#include <core/spinlock.hh>
#include <util/atomic.hh>


/// Driver base class
class driver
{
public:
	enum {
		NAME_NR = 32,
	};

protected:
	driver(const char* driver_name);
	~driver();

public:
	const char* get_name() const { return name; }

public:
	chain_node<driver> driver_ctl_chain_node;
private:
	char name[NAME_NR];  ///< driver name
};


class driver_ctl
{
	DISALLOW_COPY_AND_ASSIGN(driver_ctl);

public:
	driver_ctl();

	cause::t append_driver(driver* drv);
	cause::t remove_driver(driver* drv);
	cause::pair<driver*> search_driver(const char* name);

private:
	chain<driver, &driver::driver_ctl_chain_node> driver_chain;
	spin_lock driver_chain_lock;
};

driver_ctl* get_driver_ctl();


#endif  // include guard

