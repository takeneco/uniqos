/// @file   mempool_ctl.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_CTL_HH_
#define INCLUDE_MEMPOOL_CTL_HH_

#include "memcache.hh"


class mempool_ctl
{
	typedef bibochain<mem_pool, &mem_pool::chain_hook> mempool_chain;

public:
	mempool_ctl() {}

	cause::stype init();

	mem_pool* shared_mem_pool(u32 objsize);

private:
	mem_pool* find_shared(u32 objsize);
	mem_pool* create_shared(u32 objsize);

private:
	mem_pool* own_mempool;
	mempool_chain shared_chain;
};

cause::stype mempool_init();


#endif  // include guard

