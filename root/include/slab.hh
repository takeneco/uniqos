/// @file   slab.hh
/// @brief  slab interface.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_SLAB_HH_
#define INCLUDE_SLAB_HH_

#include "arch.hh"
#include "chain.hh"


/// slab interface of allocate and free.
class mem_cache
{
	friend cause::stype slab_init();

	class slab
	{
	public:
		typedef u16 objindex;
		struct obj_desc {
			objindex next_free;
		};

	private:
		enum { ENDOBJ = 0xffff };

		objindex first_free;
		objindex alloc_count;
		u8* obj_head;

		bichain_link<slab> chain_link_;

		obj_desc& ref_obj_desc(objindex i) {
			return (reinterpret_cast<obj_desc*>(this + 1))[i];
		}
		u8* onslab_get_obj_head(const mem_cache& parent);

	public:
		  chain_link<slab>&   chain_hook() { return chain_link_; }
		bichain_link<slab>& bichain_hook() { return chain_link_; }

		slab() :
			first_free(0),
			alloc_count(0),
			chain_link_()
		{}

		bool is_full() const {
			return first_free == ENDOBJ;
		}
		bool is_free() const {
			return alloc_count == 0;
		}

		void init_onslab(mem_cache& parent);
		void* alloc(mem_cache& parent);
		void free(mem_cache& parent, void* obj);
	};

	typedef   chain<slab, &slab::  chain_hook> slab_chain;
	typedef bichain<slab, &slab::bichain_hook> slab_bichain;
	slab_chain   free_chain;
	slab_bichain partial_chain;
	slab_bichain full_chain;

	slab::objindex slab_maxobjs;
	u32 obj_size;
	arch::page::TYPE slab_page_type;
	uptr             slab_page_size;

	bichain_link<mem_cache> chain_link_;

public:
	bichain_link<mem_cache>& chain_hook() { return chain_link_; }

	mem_cache(
	    u32 _obj_size,
	    arch::page::TYPE pt = arch::page::INVALID,
	    bool force_offslab = false);

	slab::objindex get_slab_maxobjs() const { return slab_maxobjs; }
	u32 get_obj_size() const { return obj_size; }

	/// only used by slab_init()
	void insert_slab_to_partial_chain(slab* s) {
		partial_chain.insert_head(s);
	}

	void* alloc();
	void free(void* ptr);

private:
	static arch::page::TYPE auto_page_type(u32 objsize);

	slab* new_slab();
};


class memcache_ctrl
{
	mem_cache* own_memcache;
public:
};


#endif  // include guard

