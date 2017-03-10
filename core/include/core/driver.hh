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
	char name[NAME_NR];  ///< driver name
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

public:
	class iter_by_type
	{
		driver_ctl* owner;
		driver* cur;
		driver::TYPE type;
	public:
		iter_by_type(driver_ctl* ctl, driver::TYPE filter_type) :
			owner(ctl), cur(nullptr), type(filter_type)
		{
			owner->driver_chain_lock.lock();
		}
		~iter_by_type()
		{
			owner->driver_chain_lock.unlock();
		}
		iter_by_type& begin() {
			cur = owner->driver_chain.front();
			while (cur && cur->get_type() != type)
				cur = owner->driver_chain.next(cur);
			return *this;
		}
		iter_by_type end() {
			return iter_by_type(owner, type);
		}
		bool operator == (const iter_by_type& itr) const {
			return cur == itr.cur;
		}
		bool operator != (const iter_by_type& itr) const {
			return cur != itr.cur;
		}
		driver* operator * () {
			return cur;
		}
		iter_by_type& operator ++ () {
			for (;;) {
				cur = owner->driver_chain.next(cur);
				if (!cur || cur->get_type() == type)
					break;
			}
			return *this;
		}
	};
	/// この関数の戻り値は右辺値参照として最適化される必要がある。
	iter_by_type enum_by_type(driver::TYPE type) {
		return iter_by_type(this, type);
	}

private:
	chain<driver, &driver::driver_ctl_chain_node> driver_chain;
	spin_lock driver_chain_lock;
};

driver_ctl* get_driver_ctl();


#endif  // include guard

