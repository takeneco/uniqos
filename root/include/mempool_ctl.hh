/// @file   mempool_ctl.hh
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_CTL_HH_
#define INCLUDE_MEMPOOL_CTL_HH_

#include <mempool.hh>


class mempool_ctl
{
	friend cause::type mempool_init();

	mempool_ctl(mempool* _mempool_mp, mempool* _node_mp, mempool* _own_mp);
	cause::type init();

	typedef bibochain<mempool, &mempool::chain_hook> mempool_chain;

public:
	enum PAGE_STYLE {
		ENTRUST,
		ONPAGE,
		OFFPAGE,
	};

public:
	cause::type shared_mempool(u32 objsize, mempool** mp);
	void release_shared_mempool(mempool* mp);

	cause::type exclusived_mempool(
	    u32 objsize,
	    arch::page::TYPE page_type,
	    PAGE_STYLE page_style,
	    mempool** mp);

	void* shared_alloc(u32 bytes);
	void shared_dealloc(void* mem);

	void dump(log_target& lt);

private:
	cause::type init_shared();

	mempool* find_shared(u32 objsize);
	cause::type create_shared(u32 objsize, mempool** new_mp);

	static cause::type decide_params(
	    u32* objsize,
	    arch::page::TYPE* page_type,
	    PAGE_STYLE* page_style);

	static cause::stype create_mempool_ctl(mempool_ctl** mpctl);

private:
	/// offpage mempool のページ源。
	mempool* offpage_mp;

	mempool_chain shared_chain;

	/// mempool の生成に使う。
	mempool* const mempool_mp;

	/// mempool::node の生成に使う。
	mempool* const node_mp;

	/// mempool_ctl の生成のために使われた mempool
	mempool* const own_mp;
};


#endif  // include guard

