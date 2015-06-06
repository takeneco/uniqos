/// @file   core/vadr_pool.hh
/// @brief  virtual address pool.

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

#ifndef CORE_VADR_POOL_HH_
#define CORE_VADR_POOL_HH_

#include <core/mempool.hh>
#include <core/pagetbl.hh>
#include <core/spinlock.hh>


class vadr_pool
{
	struct resource
	{
		chain_node<resource>  _chain_node;
		adr_range             vadr_range;
		adr_range             padr_range;
		page_flags            pageflags;
		page_level            pagelevel;
		u32                   ref_cnt;

		uptr get_vadr(uptr padr);
	};
	using resource_chain = front_chain<resource, &resource::_chain_node>;

public:
	vadr_pool(uptr _pool_start, uptr _pool_end);

public:
	cause::t setup();
	cause::t unsetup();

	cause::pair<void*> assign(
	    uptr padr, uptr bytes, page_flags flags);
	cause::t revoke(void* vadr);

private:
	cause::pair<resource*> assign_by_assign(
	    uptr padr, uptr bytes, page_flags flags);
	cause::pair<resource*> assign_by_pool(
	    uptr padr, uptr bytes, page_flags flags);
	cause::pair<resource*> cut_resource(uptr page_size);
	cause::t merge_pool(resource* res);

private:
	uptr pool_start;
	uptr pool_end;

	resource_chain pool_chain;
	resource_chain assign_chain;
	spin_lock lock;

	mempool* resource_mp;
};


#endif  // include guard

