/// @file   mempool_ctl.hh
//
// (C) 2011-2014 KATO Takeshi
//

#ifndef CORE_MEMPOOL_CTL_HH_
#define CORE_MEMPOOL_CTL_HH_

#include <core/mempool.hh>
#include <core/spinlock.hh>


class mempool_ctl
{
	friend cause::t mempool_setup();
	friend cause::t mempool_post_setup();

	mempool_ctl(mempool* _mempool_mp, mempool* _node_mp, mempool* _own_mp);
	cause::t setup();
	cause::t post_setup();

	typedef bibochain<mempool, &mempool::chain_hook> mempool_chain;

public:
	cause::pair<mempool*> acquire_shared_mempool(u32 objsize);
	void release_shared_mempool(mempool* mp);

	cause::pair<mempool*> exclusived_mempool(
	    u32 objsize,
	    arch::page::TYPE page_type,
	    mempool::PAGE_STYLE page_style);

	void* shared_allocate(u32 bytes);
	void shared_deallocate(void* mem);

	void dump(output_buffer& ob);

private:
	cause::t init_shared();

	mempool* find_shared(u32 objsize);
	cause::t create_shared(u32 objsize, mempool** new_mp);

	static cause::t decide_params(
	    u32* objsize,
	    arch::page::TYPE* page_type,
	    mempool::PAGE_STYLE* page_style);

	static cause::pair<mempool_ctl*> create_mempool_ctl();

private:
	/// offpage mempool のページ源。
	mempool* offpage_mp;

	mempool_chain shared_chain;
	mempool_chain exclusived_chain;

	/// mempool の生成に使う。
	mempool* const mempool_mp;

	/// mempool::node の生成に使う。
	mempool* const node_mp;

	/// mempool_ctl の生成のために使われた mempool
	mempool* const own_mp;

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

