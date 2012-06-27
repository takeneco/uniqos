/// @file  mempool.cc
/// @brief Memory pooler.

//  Uniqos  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <mempool_ctl.hh>

#include <log.hh>
#include <page.hh>
#include <placement_new.hh>
#include <cpu_node.hh>


mempool::mempool(u32 _obj_size, arch::page::TYPE ptype, mempool* _page_pool)
:   obj_size(normalize_obj_size(_obj_size)),
    page_type(ptype == arch::page::INVALID ? auto_page_type(obj_size) : ptype),
    page_size(arch::page::size_of_type(page_type)),
    page_objs((page_size - sizeof (page)) / obj_size),
    total_obj_size(page_objs * obj_size),
    alloc_count(0),
    page_count(0),
    freeobj_count(0),
    shared_count(0),
    page_pool(_page_pool)
{
}

cause::stype mempool::destroy()
{
	for (;;) {
		memobj* obj = free_objs.remove_head();
		back_to_page(obj);
		--freeobj_count;
	}

	if (alloc_count != 0 ||
	    page_count != 0 ||
	    freeobj_count != 0 ||
	    shared_count != 0 ||
	    free_objs.head() != 0 ||
	    free_pages.head() != 0 ||
	    full_pages.head() != 0)
	{
		log()("FAULT:")
		(__FILE__, __LINE__, __func__)("Bad operation.")()
		("| alloc_count: ").u(alloc_count)
		(", page_count: ").u(page_count)()
		("| freeobj_count: ").u(freeobj_count)
		(", shared_count: ").u(shared_count)()
		("| free_objs.head(): ")(free_objs.head())()
		("| free_pages.head(): ")(free_pages.head())()
		("| full_pages.head(): ")(full_pages.head())();

		return cause::FAIL;
	}

	return cause::OK;
}

void* mempool::alloc()
{
	cpu_node* proc = get_cpu_node();
	proc->preempt_disable();

	void* r = _alloc();

	proc->preempt_enable();

	return r;
}

void mempool::dealloc(void* ptr)
{
	cpu_node* proc = get_cpu_node();
	proc->preempt_disable();

	_dealloc(ptr);

	proc->preempt_enable();
}

void mempool::collect_free_pages()
{
	const sptr SAVE_FREEOBJS = 64;

	const sptr n = freeobj_count - SAVE_FREEOBJS;
	for (sptr i = 0; i < n; ++i) {
		memobj* obj = free_objs.remove_head();
		back_to_page(obj);
		--freeobj_count;
	}
}

void mempool::dump(log_target& lt)
{
	lt.u(obj_size, 16)(" ").
	   u(alloc_count, 16)(" ").
	   u(page_count, 16)(" ").
	   u(freeobj_count, 16)();

	lt("---- free_pages ----")();

	for (page* pg = free_pages.head(); pg; pg = free_pages.next(pg)) {
		pg->dump(lt);
	}
}

void* mempool::node::alloc()
{
	memobj* obj = free_objs.remove_head();
	if (obj) {
		--freeobj_cnt;
	} else {
		page* pg = free_pages.head();
		if (pg == 0)
			return 0;

		obj = pg->alloc();

		if (pg->is_full()) {
			free_pages.remove_head(); // remove pg
			full_pages.insert_head(pg);
		}
	}

	++alloc_cnt;

	return obj;
}

void mempool::node::dealloc(void* ptr)
{
	memobj* obj = new (ptr) memobj;

	free_objs.insert_head(obj);

	--alloc_cnt;
	++freeobj_cnt;
}

void mempool::node::supply_page(page* new_page)
{
	free_pages.insert_head(new_page);
}

void mempool::node::include_dirty_page(page* page)
{
	if (page->is_full())
		free_pages.insert_head(page);
	else
		full_pages.insert_head(page);

	++page_cnt;
	alloc_cnt += page->count_alloc();
}

/// @param[in] owner  ページのサイズ情報へのアクセスとページの開放に使用する
void mempool::node::back_to_page(memobj* obj, mempool* owner)
{
	for (page* pg = free_pages.head(); pg; pg = free_pages.next(pg)) {
		if (pg->free(*owner, obj)) {
			if (pg->is_free()) {
				free_pages.remove(pg);
				owner->delete_page(pg);
			}
			return;
		}
	}

	for (page* pg = full_pages.head(); pg; pg = full_pages.next(pg)) {
		if (pg->free(*owner, obj)) {
			full_pages.remove(pg);
			free_pages.insert_head(pg);
			return;
		}
	}

	log()("!!!FAULT:")(__func__)("() no matched page: obj:")(obj)();
}

arch::page::TYPE mempool::auto_page_type(u32 objsize)
{
	arch::page::TYPE r = arch::page::type_of_size(objsize * 4);
	if (r == arch::page::INVALID)
		r = arch::page::type_of_size(objsize);

	return r;
}

u32 mempool::normalize_obj_size(u32 objsize)
{
	u32 r = max<u32>(objsize, sizeof (page));

	r = up_align<u32>(r, sizeof (cpuword));

	return r;
}

void* mempool::_alloc()
{
	const int cpuid = arch::get_cpu_id();

	node* nd = mempool_nodes[cpuid];

	void* r = nd->alloc();
	if (!r) {
		page* pg = new_page();
		if (UNLIKELY(!pg))
			return 0;

		nd->supply_page(pg);

		r = nd->alloc();
	}

	if (r)
		++alloc_count;

	return r;
}

void mempool::_dealloc(void* ptr)
{
	const int cpuid = arch::get_cpu_id();

	mempool_nodes[cpuid]->dealloc(ptr);

	--alloc_count;
}

void mempool::attach(page* pg)
{
	if (pg->is_full())
		full_pages.insert_head(pg);
	else
		free_pages.insert_head(pg);
}

mempool::page* mempool::new_page()
{
	const int cpuid = arch::get_cpu_id();

	return new_page(cpuid);
}

mempool::page* mempool::new_page(int cpuid)
{
	uptr padr;
	if (page_alloc(cpuid, page_type, &padr) != cause::OK)
		return 0;

	void* mem = arch::map_phys_adr(padr, page_size);

	page* pg;

	if (page_pool) {
		pg = new (page_pool->alloc()) page;
		if (UNLIKELY(!pg)) {
			arch::page::free(page_type, padr);
			return 0;
		}

		pg->init_offpage(*this, mem);
	} else {
		pg = new (mem) page;
		pg->init_onpage(*this);
	}

	++page_count;

	return pg;
}

void mempool::delete_page(page* pg)
{
	operator delete(pg, pg);

	if (page_pool) {
		const uptr adr =
		    arch::unmap_phys_adr(pg->get_memory(), page_size);
		page_pool->dealloc(pg);
		page_dealloc(page_type, adr);
	} else {
		const cause::stype r = page_dealloc(
		    page_type, arch::unmap_phys_adr(pg, page_size));
		if (is_fail(r)) {
			log()(__func__)("() failed page free:").u(r)
			     (" page:")(pg)();
		}
	}

	--page_count;
}

void mempool::back_to_page(memobj* obj)
{
	for (page* pg = free_pages.head(); pg; pg = free_pages.next(pg)) {
		if (pg->free(*this, obj)) {
			if (pg->is_free()) {
				free_pages.remove(pg);
				delete_page(pg);
			}
			return;
		}
	}

	for (page* pg = full_pages.head(); pg; pg = full_pages.next(pg)) {
		if (pg->free(*this, obj)) {
			full_pages.remove(pg);
			free_pages.insert_head(pg);
			return;
		}
	}

	log()("FAULT:")(__func__)("() no matched page: obj:")(obj)();
}

void mempool::set_node(int i, node* nd)
{
	mempool_nodes[i] = nd;
}


// mempool::page


/// @brief initialize as onpage.
void mempool::page::init_onpage(const mempool& pool)
{
	memory = onpage_get_memory();

	init(pool);
}

/// @brief initialize as offpage.
void mempool::page::init_offpage(const mempool& pool, void* _memory)
{
	memory = reinterpret_cast<u8*>(_memory);

	init(pool);
}

/// @brief  memobj を1つ確保する。
//
/// 空き memobj が無い mempool::page で alloc() を呼び出してはならない。
mempool::memobj* mempool::page::alloc()
{
	++alloc_count;

	return free_chain.remove_head();
}

/// @brief  memobj を1つ開放する。
bool mempool::page::free(const mempool& pool, memobj* obj)
{
	const void* obj_beg = memory;
	const void* obj_end = memory + pool.get_total_obj_size();

	if (!(obj_beg <= obj && obj < obj_end))
		return false;

	--alloc_count;

	free_chain.insert_head(obj);

	return true;
}

void mempool::page::dump(log_target& lt)
{
	lt("memory:")(memory)(", alloc_count:").u(alloc_count)();

	lt("{");
	for (memobj* obj = free_chain.head(); obj; obj = free_chain.next(obj)) {
		lt(obj)(",");
	}
	lt("}")();
}

void mempool::page::init(const mempool& pool)
{
	const u32 objsize = pool.get_obj_size();

	for (int i = pool.get_page_objs() - 1; i >= 0; --i) {
		void* obj = &memory[objsize * i];
		free_chain.insert_head(new (obj) memobj);
	}
}

