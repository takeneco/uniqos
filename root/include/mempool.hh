/// @file   mempool.hh
/// @brief  mempool interface.
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_HH_
#define INCLUDE_MEMPOOL_HH_

#include "arch.hh"
#include "chain.hh"


class mempool
{
	friend class mempool_ctl;

public:
	mempool(
	    u32 _obj_size,
	    arch::page::TYPE ptype = arch::page::INVALID);

	u32 get_obj_size() const { return obj_size; }
	u32 get_page_objs() const { return page_objs; }
	uptr get_total_obj_size() const { return total_obj_size; }

	void* alloc();
	void free(void* ptr);
	void collect_free_pages();

	bichain_node<mempool>& chain_hook() { return _chain_node; }

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

		void init_onpage(const mempool& parent);
		memobj* alloc();
		bool free(const mempool& pool, memobj* obj);

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
	void delete_page(page* pg);
	void back_to_page(memobj* obj);

private:
	const u32        obj_size;
	arch::page::TYPE page_type;
	uptr             page_size;
	uptr             total_obj_size;
	u32              page_objs;  ///< ページの中にあるオブジェクト数

	sptr             alloc_count;
	sptr             page_count;
	sptr             freeobj_count;

	typedef chain<memobj, &memobj::chain_hook> obj_chain;
	obj_chain free_objs;

	typedef bichain<page, &page::bichain_hook> page_bichain;
	page_bichain free_pages;
	page_bichain full_pages;

	bichain_node<mempool> _chain_node;
};

mempool* mempool_create_shared(u32 objsize);


#endif  // include guard

