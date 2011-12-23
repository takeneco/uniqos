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
	mempool_ctl() {}

	cause::stype init();

	mempool* shared_mempool(u32 objsize);

private:
	mempool* find_shared(u32 objsize);
	mempool* create_shared(u32 objsize);

private:
	mempool* own_mempool;
	mempool_chain shared_chain;
};

cause::stype mempool_init();


#endif  // include guard

