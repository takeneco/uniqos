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
	enum TYPE {
		TYPE_BUS = 0,
		TYPE_BLOCK = 1,
		TYPE_NR,
	};
	enum {
		NAME_NR = 12,
	};

protected:
	device(TYPE dev_type);

public:
	TYPE get_type() const { return type; }
	void set_name(const char* device_name);
	const char* get_name() const;
	const char* get_class() const;
	void inc_ref() { ref_cnt.inc(); }
	void dec_ref() { ref_cnt.dec(); }

private:
	atomic<u16> ref_cnt;  ///< referentce counter
	TYPE type;
	char name[NAME_NR + 1];   ///< device name for identify
	chain_node<device> device_ctl_chain_node;

protected:
	const char* dev_class;          ///< device class

public:
	using iterative_chain = chain<device, &device::device_ctl_chain_node>;
};


/// @brief Bus device base class
class bus_device : public device
{
protected:
	bus_device() :
		device(TYPE_BUS)
	{}
};

/// @brief Block device base class
class block_device : public device
{
protected:
	block_device() :
		device(TYPE_BLOCK)
	{}
};

class device_ctl
{
	friend cause::t device_ctl_setup();

private:
	device_ctl();

public:
	//cause::t append_bus_device(bus_device* dev);
	cause::t append_device(device* dev);
	//cause::t remove_bus_device(bus_device* dev);
	cause::t remove_device(device* dev);
	locked_chain_iterator<device::iterative_chain, device>
	    each_devices();

private:
	device::iterative_chain dev_chain;
	spin_rwlock dev_chain_lock;
};

device_ctl* get_device_ctl();


#endif  // CORE_DEVICE_HH_

