/// @file   core/driver.hh
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
#include <core/refcnt.hh>


/// Driver base class
class driver
{
	DISALLOW_COPY_AND_ASSIGN(driver);

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
	refcnt<> refs;
private:
	TYPE type;
	char name[NAME_NR + 1];  ///< driver name
};


class driver_ctl
{
	DISALLOW_COPY_AND_ASSIGN(driver_ctl);

	friend cause::t driver_ctl_setup();

	driver_ctl();

public:
	cause::t register_driver(driver* drv);
	cause::t unregister_driver(driver* drv);
	cause::pair<driver*> get_driver_by_name(const char* name);

private:
	chain<driver, &driver::driver_ctl_chain_node> driver_chain;
	spin_rwlock driver_chain_lock;

public:
	class type_filter
	{
		const driver::TYPE type;

	public:
		type_filter(const type_filter& other) :
			type(other.type)
		{
		}
		type_filter(driver::TYPE filter_type) :
			type(filter_type)
		{
		}

		bool filter(driver* drv) const {
			return drv->get_type() == type;
		}
	};
	using driver_type_filter_t =
	    chain_filter<decltype (driver_chain), type_filter>;
	spin_rlocked_pair<driver_type_filter_t, false>
	    filter_by_type(driver::TYPE type);
};

driver_ctl* get_driver_ctl();


#endif  // CORE_DRIVER_CTL_HH_

