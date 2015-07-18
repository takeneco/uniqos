/// @file   core/device.hh
/// @brief  device interface declaration.

//  Uniqos  --  Unique Operating System
//  (C) 2014-2015 KATO Takeshi
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

#ifndef CORE_DEVICE_HH_
#define CORE_DEVICE_HH_

#include <core/basic.hh>
#include <core/spinlock.hh>
#include <util/foreach.hh>


/// @brief  Device base class
class device
{
public:
	enum {
		NAME_NR = 32 - sizeof (atomic<u16>),
	};

public:
	device(const char* device_name);

public:
	const char* get_name() const { return name; }
	void inc_ref() { ref_cnt.inc(); }
	void dec_ref() { ref_cnt.dec(); }

private:
	atomic<u16> ref_cnt;  ///< referentce counter
	char name[NAME_NR];   ///< device name
	chain_node<device> device_ctl_chain_node;

public:
	using iterative_chain = chain<device, &device::device_ctl_chain_node>;
};


/// @brief Bus device base class
class bus_device : public device
{
public:
	bus_device(const char* device_name) :
		device(device_name)
	{}
};


class device_ctl
{
	friend cause::t device_ctl_setup();

private:
	device_ctl();

public:
	cause::t append_bus_device(bus_device* dev);
	cause::t remove_bus_device(bus_device* dev);
	locked_chain_iterator<device::iterative_chain, bus_device>
	    each_bus_devices();

private:
	device::iterative_chain bus_chain;
	spin_rwlock bus_chain_lock;
};

device_ctl* get_device_ctl();


#endif  // include guard

