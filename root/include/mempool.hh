/// @file   mempool.hh
/// @brief  mempool interface.
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_HH_
#define INCLUDE_MEMPOOL_HH_

#include <arch.hh>
#include <chain.hh>
#include <config.h>


class log_target;
class mempool_ctl;

class mempool
{
	friend class mempool_ctl;

public:
	mempool(u32 _obj_size,
	        arch::page::TYPE ptype = arch::page::INVALID,
	        mempool* _page_pool = 0);
	cause::stype destroy();

	u32 get_obj_size() const { return obj_size; }
	u32 get_page_objs() const { return page_objs; }
	uptr get_total_obj_size() const { return total_obj_size; }
	sptr get_alloc_count() const { return alloc_count; }

	void* alloc();
	void dealloc(void* ptr);
	void collect_free_pages();

	sptr inc_shared_count() { return ++shared_count; }
	sptr dec_shared_count() { return --shared_count; }
	sptr get_shared_count() const { return shared_count; }

	void dump(log_target& lt);

	bichain_node<mempool>& chain_hook() { return _chain_node; }

private:
	class memobj
	{
		chain_node<memobj> _chain_node;
	public:
		chain_node<memobj>& chain_hook() { return _chain_node; }
	};
	typedef chain<memobj, &memobj::chain_hook> obj_chain;

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
		u8* get_memory() {
			return memory;
		}
		u32 count_alloc() const {
			return alloc_count;
		}

		void init_onpage(const mempool& pool);
		void init_offpage(const mempool& pool, void* _memory);
		memobj* alloc();
		bool free(const mempool& pool, memobj* obj);

		void dump(log_target& lt);

		bichain_node<page>& bichain_hook() { return _chain_node; }

	private:
		u8* onpage_get_memory() {
			return reinterpret_cast<u8*>(this + 1);
		}
		void init(const mempool& pool);

	private:
		chain<memobj, &memobj::chain_hook> free_chain;
		u32 alloc_count;

		u8* memory;

		bichain_node<page> _chain_node;
	};
	typedef bichain<page, &page::bichain_hook> page_bichain;

	class node
	{
		friend class mempool_ctl;
	public:
		void* alloc();
		void dealloc(void* ptr);

		void supply_page(page* new_page);

	private:
		void include_dirty_page(page* page);
		void back_to_page(memobj* obj, mempool* page_deleter);

	private:
		sptr alloc_cnt;
		sptr page_cnt;
		sptr freeobj_cnt;

		obj_chain free_objs;

		page_bichain free_pages;
		page_bichain full_pages;
	};

private:
	static arch::page::TYPE auto_page_type(u32 objsize);
	static u32 normalize_obj_size(u32 objsize);

	void* _alloc();
	void _dealloc(void* ptr);

	void attach(page* pg);
	page* new_page();
	page* new_page(int cpuid);
	void delete_page(page* pg);
	void back_to_page(memobj* obj);

	void set_node(int i, node* nd);

private:
	const u32              obj_size;
	const arch::page::TYPE page_type;
	const uptr             page_size;
	const u32              page_objs;  ///< ページの中にあるオブジェクト数
	const uptr             total_obj_size;

	sptr             alloc_count;
	sptr             page_count;
	sptr             freeobj_count;
	sptr             shared_count;

	obj_chain free_objs;

	page_bichain free_pages;
	page_bichain full_pages;

	mempool* const page_pool;

	bichain_node<mempool> _chain_node;

	node* mempool_nodes[CONFIG_MAX_CPUS];
};

extern "C" cause::type mempool_create_shared(u32 objsize, mempool** mp);
extern "C" void mempool_release_shared(mempool* mp);
void* mem_alloc(u32 bytes);
void mem_dealloc(void* mem);

inline void* operator new (uptr, mempool* mp) { return mp->alloc(); }


#endif  // include guard

