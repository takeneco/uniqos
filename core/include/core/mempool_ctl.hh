/// @file   mempool_ctl.hh
//
// (C) 2011-2014 KATO Takeshi
//

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
	cause::t teardown();
	cause::t post_setup();

	typedef bibochain<mempool, &mempool::chain_hook> mempool_chain;

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
	cause::t teardown_mp();
	cause::t setup_shared_mp();
	cause::t teardown_shared_mp();

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

	const mem_allocator::operations* get_mp_allocator_ops() const {
		return &_mp_allocator_ops;
	}

private:
	class shared_mem_allocator : public mem_allocator
	{
	public:
		shared_mem_allocator() : mem_allocator(&_ops) {}
		void init();

	private:
		operations _ops;
	};

	shared_mem_allocator _shared_mem;

	mem_allocator::operations _mp_allocator_ops;
};

mempool_ctl* get_mempool_ctl();


#endif  // include guard

