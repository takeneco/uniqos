/// @file   core/devnode.hh
/// @brief  device node interface declaration.

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

#ifndef CORE_DEVNODE_HH_
#define CORE_DEVNODE_HH_

#include <core/io_node.hh>
#include <core/mempool.hh>


typedef u32 devnode_no;

devnode_no make_dev(int major, int minor);
int get_dev_major(devnode_no dev_no);
int get_dev_minor(devnode_no dev_no);

struct major_minor
{
	major_minor() :
		major(0), minor(0)
	{}

	major_minor(const major_minor& no) :
		major(no.major), minor(no.minor)
	{}

	major_minor(int maj, int min) :
		major(maj), minor(min)
	{}

	void set(int maj, int min) {
		major = maj;
		minor = min;
	}

	bool operator == (const major_minor& y) const {
		return major == y.major && minor == y.minor;
	}

	int major;
	int minor;
};

class dev_node_ctl
{
	struct minor_set
	{
		minor_set(int min, io_node* ion) :
			minor(min),
			node(ion)
		{}

		int minor;
		io_node* node;
		chain_node<minor_set> dev_node_ctl_chain_node;
	};
	using minor_set_chain =
	    front_chain<minor_set, &minor_set::dev_node_ctl_chain_node>;

	struct major_set
	{
		major_set(int maj) :
			major(maj)
		{}

		int major;
		minor_set_chain minor_chain;
		chain_node<major_set> dev_node_ctl_chain_node;
	};
	using major_set_chain =
	    front_chain<major_set, &major_set::dev_node_ctl_chain_node>;

public:
	dev_node_ctl();
	~dev_node_ctl();

	cause::t setup();

	cause::pair<io_node*> search(int maj, int min);
	cause::pair<int> assign_major(int maj_from = 1, int maj_to = 0xffff);
	cause::pair<int> assign_minor(io_node* ion,
	     int maj, int min_from, int min_to);

private:
	cause::pair<major_set*> search_major(int maj);
	cause::pair<minor_set*> search_minor(major_set* major, int min);

public:
	major_set_chain major_set_pool;

	mempool* minor_set_mp;
	mempool* major_set_mp;
};

cause::pair<io_node*> devnode_search(int maj, int min);

cause::pair<devnode_no> devnode_assign_minor(
    io_node* ion,
    int maj,
    int min_from,
    int min_to);


#endif  // CORE_DEVNODE_HH_

