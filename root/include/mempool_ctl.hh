/// @file   mempool_ctl.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_CTL_HH_
#define INCLUDE_MEMPOOL_CTL_HH_

#include "mempool.hh"


class mempool_ctl
{
	typedef bibochain<mempool, &mempool::chain_hook> mempool_chain;

public:
	enum PAGE_STYLE {
		ENTRUST,
		ONPAGE,
		OFFPAGE,
	};

public:
	mempool_ctl() {}

	cause::stype init();

	mempool* shared_mempool(u32 objsize);
	mempool* exclusived_mempool(
	    u32 objsize,
	    arch::page::TYPE page_type = arch::page::INVALID,
	    PAGE_STYLE page_style = ENTRUST);
	void* shared_alloc(u32 bytes);

private:
	cause::stype init_heap();
	mempool* find_shared(u32 objsize);
	mempool* create_shared(u32 objsize);
	static void decide_params(
	    u32* objsize, arch::page::TYPE* page_type, PAGE_STYLE* page_style);

private:
	mempool* own_mempool;
	mempool* offpage_pool;
	mempool_chain shared_chain;
};

cause::stype mempool_init();


#endif  // include guard

