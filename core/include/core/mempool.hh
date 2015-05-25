/// @file   core/mempool.hh
/// @brief  mempool interface declaration.

//  Uniqos  --  Unique Operating System
//  (C) 2011-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CORE_MEMPOOL_HH_
#define CORE_MEMPOOL_HH_

#include <arch/pagetable.hh>
#include <config.h>
#include <core/new_ops.hh>
#include <util/spinlock.hh>


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
/// mempool_ctl 経由で mempool を生成すれば、node は mempool_ctl::node_mp
/// から生成される。
class mempool
{
	friend class mempool_ctl;

private:
	struct memobj
	{
		forward_chain_node<memobj> chain_node;
	};
	typedef front_forward_chain<memobj, &memobj::chain_node> obj_chain;

	class page
	{
	public:
		page();

		bool is_full() const;
		bool is_free() const;
		u8* get_memory();

		void init_as_onpage(const mempool& pool);
		void init_as_offpage(const mempool& pool, void* _memory);
		memobj* acquire();
		bool release(const mempool& pool, memobj* obj);

		void dump(output_buffer& lt);

	private:
		u8* onpage_get_memory() {
			return reinterpret_cast<u8*>(this + 1);
		}
		void init(const mempool& pool);

	private:
		obj_chain free_chain;
		u32 acquire_cnt;

		u8* memory;

	public:
		chain_node<page> chain_node;
	};
	typedef front_chain<page, &page::chain_node> page_bichain;

	class node
	{
		friend class mempool_ctl;

	private:
		node();

	public:
		void* acquire();
		void release(void* ptr);

		sptr get_freeobj_cnt() const { return freeobj_cnt; }

		void push_page(page* new_page);
		memobj* pop_freeobj(mempool* owner);
		void collect_free_pages(mempool* owner);
		bool back_to_page(memobj* obj, mempool* owner);

	private:
		void import_dirty_page(page* page);

	private:
		sptr freeobj_cnt;

		obj_chain free_objs;

		page_bichain free_pages;
		page_bichain full_pages;

		spin_lock lock;
	};

public:
	enum PAGE_STYLE {
		ENTRUST,
		ONPAGE,
		OFFPAGE,
	};

private:
	mempool(u32 _obj_size,
	        arch::page::TYPE ptype = arch::page::INVALID,
	        mempool* _page_pool = 0);
	void setup_mem_allocator(const mem_allocator::interfaces* ops);
	cause::t destroy();

public:
	static cause::pair<mempool*> create_exclusive(
	    u32 objsize, arch::page::TYPE page_type, PAGE_STYLE page_style);
	static cause::t              destroy_exclusive(mempool* mp);

	static cause::pair<mempool*> acquire_shared(u32 objsize);
	static void                  release_shared(mempool* mp);

	u32 get_obj_size() const { return obj_size; }
	uptr get_page_size() const { return page_size; }
	u32 get_page_objs() const { return page_objs; }
	sptr get_alloc_cnt() const { return alloc_cnt.load(); }

	cause::pair<void*> acquire();
	cause::pair<void*> acquire(cpu_id_t cpuid);
	cause::t release(void* ptr);

	void* alloc();             ///< TODO:DUPLICATED
	void* alloc(cpu_id cpuid); ///< TODO:DUPLICATED
	void dealloc(void* ptr);   ///< TODO:DUPLICATED

	void collect_free_pages();
	void collect_one_freeobj();
	bool collect_all_freeobjs();

	void inc_shared_count() { shared_count.add(1); }
	void dec_shared_count() { shared_count.sub(1); }
	sptr get_shared_count() const { return shared_count.load(); }

	void set_obj_name(const char* name);

	void dump(output_buffer& ob, uint level);
	void dump_table(output_buffer& ob);

private:
	static arch::page::TYPE auto_page_type(u32 objsize);
	static u32 normalize_obj_size(u32 objsize);

	void* _alloc(cpu_id_t cpuid);
	void _dealloc(void* ptr);

	void attach(page* pg);
	page* new_page(int cpuid);
	void delete_page(page* pg);
	void back_to_page(cpu_id src_cpu, memobj* obj);

	void set_node(int i, node* nd);
	auto get_node(int i) -> node*;

private:
	const u32              obj_size;
	const arch::page::TYPE page_type;
	const uptr             page_size;
	const u32              page_objs;  ///< ページの中にあるオブジェクト数

	atomic<sptr>           alloc_cnt;
	atomic<sptr>           page_cnt;
	atomic<sptr>           shared_count;

	mempool* const page_pool;

	chain_node<mempool> chain_node;

	node* mempool_nodes[CONFIG_MAX_CPUS];

	char obj_name[32];

// mem_allocator implement
public:
	operator mem_allocator&() { return _mem_allocator; }
private:
	class mp_mem_allocator : public mem_allocator
	{
		DISALLOW_COPY_AND_ASSIGN(mp_mem_allocator);
		friend class mem_allocator;

	public:
		mp_mem_allocator() {}
		void init(const mem_allocator::interfaces* _ifs, mempool* _mp);

	private:
		cause::pair<void*> on_Allocate(uptr bytes);
		cause::t on_Deallocate(void* p);
		cause::pair<uptr> on_GetSize(void* p);

	private:
		mempool* mp;
	};
	mp_mem_allocator _mem_allocator;
};

void* mem_alloc(u32 bytes);
void mem_dealloc(void* mem);


#endif  // include guard

