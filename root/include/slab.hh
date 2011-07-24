/// @file   slab.hh
/// @brief  slab interface.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_SLAB_HH_
#define INCLUDE_SLAB_HH_

#include "basic_types.hh"
#include "chain.hh"


/// slab interface of allocate and free.
class slab_alloc
{
	typedef bichain<slab, &slab::chain_hook> slab_chain;
	slab_chain partial_chain;
	slab_chain free_chain;
	slab_chain full_chain;

	u32 obj_size;
	u32 slab_size;

	bichain_link<slab_alloc> chain_link_;

public:
	bichain_link<slab_alloc>& chain_hook() { return chain_link_; }

	slab_alloc(u32 obj_size_, u32 slab_size_);

	/// only used by slab_init()
	void insert_slab_to_partial_chain(slab* s) {
		partial_chain.insert_head(s);
	}

	void* alloc();
	void free(void*);
};


#endif  // include guard

