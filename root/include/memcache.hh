/// @file   slab.hh
/// @brief  slab interface.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_SLAB_HH_
#define INCLUDE_SLAB_HH_

#include "arch.hh"
#include "chain.hh"

#include "log.hh"


/// slab interface of allocate and free.
class mem_cache
{
	friend class memcache_control;

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

		bichain_node<slab> _chain_node;

		obj_desc& ref_obj_desc(objindex i) {
			return (reinterpret_cast<obj_desc*>(this + 1))[i];
		}
		u8* onslab_get_obj_head(const mem_cache& parent);

	public:
		  chain_node<slab>&   chain_hook() { return _chain_node; }
		bichain_node<slab>& bichain_hook() { return _chain_node; }

		slab() :
			first_free(0),
			alloc_count(0),
			_chain_node()
		{}

		bool is_full() const {
			return first_free == ENDOBJ;
		}
		bool is_free() const {
			return alloc_count == 0;
		}

		void init_onslab(mem_cache& parent);
		void* alloc(mem_cache& parent);
		bool back(mem_cache& parent, void* obj);

		void dump(log_target& log);
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

	bichain_node<mem_cache> _chain_node;

	enum { FREE_OBJS_LEN = 64 };
	u16 free_objs_avail;
	void* free_objs[FREE_OBJS_LEN];

public:
	bichain_node<mem_cache>& chain_hook() { return _chain_node; }

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

	void attach(slab* s);
	void* alloc();
	void free(void* ptr);

private:
	static arch::page::TYPE auto_page_type(u32 objsize);

	slab* new_slab();
	void back_slab();

	void dump(log_target& log);
};

mem_cache* shared_mem_cache(u32 obj_size);


class mem_pool
{
	friend class mempool_ctl;

public:
	mem_pool(
	    u32 _obj_size,
	    arch::page::TYPE ptype = arch::page::INVALID);

	u32 get_obj_size() const { return obj_size; }
	u32 get_page_objs() const { return page_objs; }

	void* alloc();
	void free(void* ptr);

	bichain_node<mem_pool>& chain_hook() { return _chain_node; }

private:
	class memobj
	{
		chain_node<memobj> _chain_node;
	public:
		chain_node<memobj>& chain_hook() { return _chain_node; }
	};

	class page
	{
	public:
		page() :
		    alloc_count(0)
		{}

		bool is_full() const {
			return free_chain.is_empty();
		}
		bool is_free() const {
			return alloc_count == 0;
		}

		void init_onpage(const mem_pool& parent);
		memobj* alloc();
		bool free(const mem_pool& parent, memobj* obj);

		  chain_node<page>&   chain_hook() { return _chain_node; }
		bichain_node<page>& bichain_hook() { return _chain_node; }

	private:
		u8* onpage_get_memory() {
			return reinterpret_cast<u8*>(this + 1);
		}

	private:
		chain<memobj, &memobj::chain_hook> free_chain;
		u8* memory;
		u32 alloc_count;

		bichain_node<page> _chain_node;
	};

private:
	static arch::page::TYPE auto_page_type(u32 objsize);
	static u32 normalize_obj_size(u32 objsize);

	void attach(page* pg);
	page* new_page();

private:
	u32              obj_size;
	arch::page::TYPE page_type;
	uptr             page_size;
	u32              page_objs;  ///< ページの中にあるオブジェクト数

	typedef chain<memobj, &memobj::chain_hook> obj_chain;
	obj_chain free_objs;

	typedef   chain<page, &page::chain_hook> page_chain;
	typedef bichain<page, &page::bichain_hook> page_bichain;
	page_chain   free_pages;
	page_bichain partial_pages;
	page_bichain full_pages;

	bichain_node<mem_pool> _chain_node;
};

mem_pool* mempool_create_shared(u32 objsize);


#endif  // include guard

