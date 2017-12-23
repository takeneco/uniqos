/// @file  core/ns.hh
/// @brief  Namespace interfaces.

//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
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

#ifndef CORE_NS_HH_
#define CORE_NS_HH_

#include <core/basic.hh>
#include <util/atomic.hh>


class generic_ns
{
	DISALLOW_COPY_AND_ASSIGN(generic_ns);

	friend class ns_ctl;

public:
	enum TYPE {
		TYPE_FS = 1,
	};
public:
	generic_ns(TYPE t);

	bool is_type(TYPE t) const { return type == t; }

public:
	//void* context;
	chain_node<generic_ns> ns_ctl_chainnode;
protected:
	atomic<uint> refs;
	TYPE         type;
};

class ns_set
{
public:
	ns_set();

public:
	generic_ns* ns_fs;
};

cause::pair<generic_ns*> create_fs_ns();
void ns_inc_ref(generic_ns* ns);
void ns_dec_ref(generic_ns* ns);


#endif  // CORE_NS_HH_

