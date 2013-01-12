/// @file   mempool.hh
/// @brief  mempool interface.
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef INCLUDE_MEMPOOL_HH_
#define INCLUDE_MEMPOOL_HH_

#include <arch.hh>
#include <chain.hh>
#include <config.h>
#include <atomic.hh>


class output_buffer;
class mempool_ctl;

/// @brief 同じサイズのメモリブロックの割り当てを管理する。
///        スラブアロケータのようなもの。
//
/// 初期化方法
/// (1) コンストラクタを呼び出す。
/// (2) cpu_node の数だけ set_node() を呼び出して node を設定する。
///     node は cpu_node に対応する領域を割り当てるべき。
///
/// mempool_ctl 経由で mempool を生成すれば、node の生成には
/// mempool_ctl::node_mp を使う。
class mempool
{
	friend class mempool_ctl;

public:
	mempool(u32 _obj_size,
	        arch::page::TYPE ptype = arch::page::INVALID,
	        mempool* _page_pool = 0);
	cause::type destroy();

	u32 get_obj_size() const { return obj_size; }
	u32 get_page_objs() const { return page_objs; }
	uptr get_total_obj_size() const { return total_obj_size; }
	sptr get_alloc_cnt() const { return alloc_cnt.load(); }

	void* alloc();
	void* alloc(cpu_id cpuid);
	void dealloc(void* ptr);
	void collect_free_pages();

	void inc_shared_count() { shared_count.add(1); }
	void dec_shared_count() { shared_count.sub(1); }
	sptr get_shared_count() const { return shared_count.load(); }

	void set_obj_name(const char* name);

	void dump(output_buffer& ob, uint level);
	void dump_table(output_buffer& ob);

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
		    alloc_cnt(0)
		{}

		bool is_full() const {
			return free_chain.is_empty();
		}
		bool is_free() const {
			return alloc_cnt == 0;
		}
		u8* get_memory() {
			return memory;
		}
		u32 count_alloc() const {
			return alloc_cnt;
		}

		void init_onpage(const mempool& pool);
		void init_offpage(const mempool& pool, void* _memory);
		memobj* alloc();
		bool free(const mempool& pool, memobj* obj);

		void dump(output_buffer& lt);

		bichain_node<page>& bichain_hook() { return _chain_node; }

	private:
		u8* onpage_get_memory() {
			return reinterpret_cast<u8*>(this + 1);
		}
		void init(const mempool& pool);

	private:
		chain<memobj, &memobj::chain_hook> free_chain;
		u32 alloc_cnt;

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

	void* _alloc(cpu_id cpuid);
	void _dealloc(void* ptr);

	void attach(page* pg);
	page* new_page(int cpuid);
	void delete_page(page* pg);
	void back_to_page(memobj* obj);

	void set_node(int i, node* nd);

private:
	const u32              obj_size;
	const arch::page::TYPE page_type;
	const uptr             page_size;
	const u32              page_objs;  ///< ページの中にあるオブジェクト数
	const uptr             total_obj_size;  ///< obj_size * page_objs

	atomic<sptr>           alloc_cnt;
	atomic<sptr>           page_cnt;
	atomic<sptr>           freeobj_cnt;
	atomic<sptr>           shared_count;

	obj_chain free_objs;

	page_bichain free_pages;
	page_bichain full_pages;

	mempool* const page_pool;

	bichain_node<mempool> _chain_node;

	node* mempool_nodes[CONFIG_MAX_CPUS];

	char obj_name[32];
};

extern "C" cause::type mempool_acquire_shared(u32 objsize, mempool** mp);
extern "C" void        mempool_release_shared(mempool* mp);
void* mem_alloc(u32 bytes);
void mem_dealloc(void* mem);

inline void* operator new (uptr, mempool* mp) throw();


#endif  // include guard

