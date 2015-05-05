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
#include <util/spinlock.hh>


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

private:
};


// TODO: move to util
template <class CHAIN, class EXPORT_TYPE=typename CHAIN::data_t>
class locked_chain_iterator
{
	class iterator;
	using data_t = typename CHAIN::data_t;

public:
	locked_chain_iterator(data_t* _start, spin_rwlock* _lock) :
		start(_start),
		lock(_lock)
	{
		if (lock)
			lock->rlock();
	}
	locked_chain_iterator(locked_chain_iterator&& src) :
		start(src.start),
		lock(src.lock)
	{
		src.lock = nullptr;
	}

	~locked_chain_iterator()
	{
		if (lock)
			lock->un_rlock();
	}

	iterator begin() {
		return iterator(start);
	}
	iterator end() {
		return iterator(nullptr);
	}

private:
	data_t* start;
	spin_rwlock* lock;
};

template <class CHAIN, class EXPORT_TYPE>
class locked_chain_iterator<CHAIN, EXPORT_TYPE>::iterator
{
	using chain_t = CHAIN;
	using export_t = EXPORT_TYPE;

public:
	iterator(data_t* start) :
		current(start)
	{}
	iterator(iterator&& src) {
		current = src.current;
	}

	export_t* operator * () {
		return static_cast<export_t*>(current);
	}
	export_t* operator ++ () {
		current = chain_t::next(current);
		return static_cast<export_t*>(current);
	}
	bool operator == (const iterator& itr) const {
		return current == itr.current;
	}
	bool operator != (const iterator& itr) const {
		return current != itr.current;
	}

private:
	data_t* current;
};

cause::t bus_device_append(bus_device* dev);
cause::t bus_device_remove(bus_device* dev);
locked_chain_iterator<device::iterative_chain, bus_device> bus_device_iterate();


#endif  // include guard

