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


class fs_driver;

/// Driver base class
class driver
{
public:
	enum {
		NAME_NR = 31,
	};
	enum TYPE {
		TYPE_UNKNOWN = 0,
		TYPE_FS,
		TYPE_NR,
	};

protected:
	driver(TYPE driver_type, const char* driver_name);
	~driver();

public:
	TYPE get_type() const { return type; }
	const char* get_name() const { return name; }

public:
	chain_node<driver> driver_ctl_chain_node;
private:
	TYPE type;
	char name[NAME_NR + 1];  ///< driver name
};


class driver_ctl
{
	DISALLOW_COPY_AND_ASSIGN(driver_ctl);

public:
	driver_ctl();

	cause::t append_driver(driver* drv) { return register_driver(drv); }
	cause::t register_driver(driver* drv);
	cause::t remove_driver(driver* drv) { return unregister_driver(drv); }
	cause::t unregister_driver(driver* drv);
	cause::pair<driver*> get_driver_by_name(const char* name);

private:
	chain<driver, &driver::driver_ctl_chain_node> driver_chain;
	spin_rwlock driver_chain_lock;

public:
	class type_filter
	{
		driver_ctl* owner;
		const driver::TYPE type;

	public:
		type_filter(type_filter& other) :
			owner(other.owner),
			type(other.type)
		{
			other.owner = nullptr;
		}
		type_filter(type_filter&& other) :
			owner(other.owner),
			type(other.type)
		{
			other.owner = nullptr;
		}
		type_filter(driver_ctl* drvctl, driver::TYPE filter_type) :
			owner(drvctl),
			type(filter_type)
		{
			owner->driver_chain_lock.rlock();
		}
		~type_filter() {
			if (owner)
				owner->driver_chain_lock.un_rlock();
		}

		bool filter(driver* drv) const {
			return drv->get_type() == type;
		}
	};
	chain_filter<decltype (driver_chain), type_filter>
	filter_by_type(driver::TYPE type) {
		return chain_filter<decltype (driver_chain), type_filter>(
		    &driver_chain, type_filter(this, type));
	}
};

driver_ctl* get_driver_ctl();


#endif  // CORE_DRIVER_CTL_HH_

