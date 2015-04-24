/// @file   mempool_ctl.hh

//  Uniqos  --  Unique Operating System
//  (C) 2011-2015 KATO Takeshi
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

#ifndef CORE_SOURCE_MEMPOOL_CTL_HH_
#define CORE_SOURCE_MEMPOOL_CTL_HH_

#include <core/mempool.hh>


class mempool_ctl
{
public:
	mempool_ctl();
	cause::t setup();
	void move_to(mempool_ctl* dest);
	void after_move();
	cause::t unsetup();
	cause::t post_setup();

	typedef chain<mempool, &mempool::chain_node> mempool_chain;

public:
	cause::pair<mempool*> acquire_shared_mempool(u32 objsize);
	void release_shared_mempool(mempool* mp);

	cause::pair<mempool*> create_exclusived_mp(
	    u32 objsize,
	    arch::page::TYPE page_type,
	    mempool::PAGE_STYLE page_style);
	cause::t destroy_exclusived_mp(mempool* mp);

	void* shared_allocate(u32 bytes);
	void shared_deallocate(void* mem);

	void dump(output_buffer& ob);

private:
	cause::t setup_mp();
	cause::t unsetup_mp();
	cause::t setup_shared_mp();
	cause::t unsetup_shared_mp();

	mempool* find_shared(u32 objsize);
	cause::t create_shared(u32 objsize, mempool** new_mp);
	void destroy_shared(mempool* mp);
	void destroy_mp(mempool* mp);

	static cause::t decide_params(
	    u32* objsize,
	    arch::page::TYPE* page_type,
	    mempool::PAGE_STYLE* page_style);

private:
	/// mempool の生成に使う。
	mempool* mempool_mp;

	/// mempool::node の生成に使う。
	mempool* node_mp;

	/// page source of offpage mempool.
	mempool* offpage_mp;

	mempool_chain shared_chain;
	mempool_chain exclusived_chain;

	spin_rwlock exclusived_chain_lock;

// mem_allocator implement
public:
	mem_allocator& shared_mem() { return _shared_mem; }

	const mem_allocator::interfaces* get_mp_allocator_ifs() const {
		return &_mp_allocator_ifs;
	}

private:
	class shared_mem_allocator : public mem_allocator
	{
	public:
		shared_mem_allocator() : mem_allocator(&_ifs) {}
		void init();

	private:
		interfaces _ifs;
	};

	shared_mem_allocator _shared_mem;

	mem_allocator::interfaces _mp_allocator_ifs;
};

mempool_ctl* get_mempool_ctl();


#endif  // include guard

