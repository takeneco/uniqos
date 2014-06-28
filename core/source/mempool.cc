/// @file  mempool.cc
/// @brief Memory pooler.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <mempool_ctl.hh>

#include <core/cpu_node.hh>
#include <global_vars.hh>
#include <core/log.hh>
#include <new_ops.hh>
#include <page.hh>
#include <core/string.hh>


mempool::mempool(u32 _obj_size, arch::page::TYPE ptype, mempool* _page_pool)
:   obj_size(normalize_obj_size(_obj_size)),
    page_type(ptype == arch::page::INVALID ? auto_page_type(obj_size) : ptype),
    page_size(arch::page::size_of_type(page_type)),
    page_objs((page_size - sizeof (page)) / obj_size),
    total_obj_size(page_objs * obj_size),
    alloc_cnt(0),
    page_cnt(0),
    freeobj_cnt(0),
    shared_count(0),
    page_pool(_page_pool)
{
	obj_name[0] = '\0';

	for (cpu_id_t i = 0; i < CONFIG_MAX_CPUS; ++i)
		mempool_nodes[i] = 0;

	_mem_allocator.init(this);
}

cause::t mempool::destroy()
{
	for (;;) {
		memobj* obj = free_objs.remove_head();
		back_to_page(obj);
		freeobj_cnt.sub(1);
	}

	if (alloc_cnt.load() != 0 ||
	    page_cnt.load() != 0 ||
	    freeobj_cnt.load() != 0 ||
	    shared_count.load() != 0 ||
	    free_objs.head() != 0 ||
	    free_pages.head() != 0 ||
	    full_pages.head() != 0)
	{
		log()("FAULT:")
		(__FILE__, __LINE__, __func__)("Bad operation.")()
		("| alloc_cnt: ").u(alloc_cnt.load())
		(", page_cnt: ").u(page_cnt.load())()
		("| freeobj_cnt: ").u(freeobj_cnt.load())
		(", shared_count: ").u(shared_count.load())()
		("| free_objs.head(): ")(free_objs.head())()
		("| free_pages.head(): ")(free_pages.head())()
		("| full_pages.head(): ")(full_pages.head())();

		return cause::FAIL;
	}

	return cause::OK;
}

cause::pair<void*> mempool::acquire()
{
	preempt_disable_section _pds;

	const cpu_id_t cpuid = arch::get_cpu_id();

	void* r = _alloc(cpuid);

	return cause::make_pair(r ? cause::OK : cause::FAIL, r);
}

cause::pair<void*> mempool::acquire(cpu_id_t cpuid)
{
	preempt_disable_section _pds;

	void* r = _alloc(cpuid);

	return cause::make_pair(r ? cause::OK : cause::FAIL, r);
}

cause::t mempool::release(void* ptr)
{
	preempt_disable_section _pds;

	_dealloc(ptr);

	return cause::OK;
}

void* mempool::alloc()
{
	preempt_disable_section _pds;

	const cpu_id_t cpuid = arch::get_cpu_id();

	void* r = _alloc(cpuid);

	return r;
}

void* mempool::alloc(cpu_id_t cpuid)
{
	preempt_disable_section _pds;

	void* r = _alloc(cpuid);

	return r;
}

void mempool::dealloc(void* ptr)
{
	preempt_disable_section _pds;

	_dealloc(ptr);
}

void mempool::collect_free_pages()
{
	/*
	const sptr SAVE_FREEOBJS = 64;

	const sptr n = freeobj_cnt.load() - SAVE_FREEOBJS;
	for (sptr i = 0; i < n; ++i) {
		memobj* obj = free_objs.remove_head();
		back_to_page(obj);
		freeobj_cnt.sub(1);
	}
	*/

	preempt_disable_section _pds;

	const cpu_id_t cpuid = arch::get_cpu_id();

	mempool_nodes[cpuid]->collect_free_pages(this);
}

void mempool::set_obj_name(const char* name)
{
	str_copy(sizeof obj_name - 1, name, obj_name);

	obj_name[sizeof obj_name - 1] = '\0';
}

void mempool::dump(output_buffer& ob, uint level)
{
	if (level >= 1) {
		ob("obj_size    : ").u(obj_size, 12)
		("\npage_type   : ").u(page_type, 12)
		(" |page_size      : ").u(page_size, 12)
		("\npage_objs   : ").u(page_objs, 12)
		(" |total_obj_size : ").u(total_obj_size, 12)
		("\nalloc_cnt   : ").s(alloc_cnt.load(), 12)
		(" |page_cnt       : ").s(page_cnt.load(), 12)
		("\nfreeobj_cnt : ").u(freeobj_cnt.load(), 12)();
	}

	if (level >= 2) {
		for (int i = 0; i < CONFIG_MAX_CPUS; ++i) {
			node* nd = mempool_nodes[i];
			ob("nd[").u(i)("]=")(nd)();
		}
	}

	if (level >= 3) {
		ob("---- free_pages ----")();

		for (page* pg = free_pages.head();
		     pg;
		     pg = free_pages.next(pg))
		{
			pg->dump(ob);
		}
	}
}

void mempool::dump_table(output_buffer& ob)
{
	ob.str(obj_name, 14)(' ').
	   u(obj_size, 11)(' ').
	   u(alloc_cnt.load(), 11)(' ').
	   u(page_cnt.load(), 11)(' ').
	   u(freeobj_cnt.load(), 11)();
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

void mempool::node::collect_free_pages(mempool* owner)
{
	const sptr SAVE_FREEOBJS = 64;

	const sptr n = freeobj_cnt - SAVE_FREEOBJS;
	for (sptr i = 0; i < n; ++i) {
		memobj* obj = free_objs.remove_head();
		back_to_page(obj, owner);
		freeobj_cnt -= 1;
	}
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

	log()("!!!!")(SRCPOS)("() no matched page: obj:")(obj)();
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
	//TODO: sizeof (memobj)
	u32 r = max<u32>(objsize, sizeof (memobj));

	r = up_align<u32>(r, sizeof (cpu_word));

	return r;
}

void* mempool::_alloc(cpu_id_t cpuid)
{
	preempt_disable_section _pds;

	node* nd = mempool_nodes[cpuid];

	void* r = nd->alloc();
	if (!r) {
		page* pg = new_page(cpuid);
		if (UNLIKELY(!pg))
			return 0;

		nd->supply_page(pg);

		r = nd->alloc();

#if CONFIG_DEBUG_VALIDATE
		if (!r)
			log()(SRCPOS)(" ?? ABNORMAL PATH");
#endif  // CONFIG_DEBUG_VALIDATE
	}

	if (r)
		alloc_cnt.add(1);

	return r;
}

void mempool::_dealloc(void* ptr)
{
	const int cpuid = arch::get_cpu_id();

	mempool_nodes[cpuid]->dealloc(ptr);

	alloc_cnt.sub(1);
}

void mempool::attach(page* pg)
{
	if (pg->is_full())
		full_pages.insert_head(pg);
	else
		free_pages.insert_head(pg);
}

mempool::page* mempool::new_page(int cpuid)
{
	uptr padr;
	if (is_fail(page_alloc(cpuid, page_type, &padr)))
		return 0;

	void* mem = arch::map_phys_adr(padr, page_size);

	page* pg;

	if (page_pool) {
		pg = new (page_pool->alloc()) page;
		if (UNLIKELY(!pg)) {
			page_dealloc(page_type, padr);
			return 0;
		}

		pg->init_offpage(*this, mem);
	} else {
		pg = new (mem) page;
		pg->init_onpage(*this);
	}

	page_cnt.add(1);

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
		const cause::t r = page_dealloc(
		    page_type, arch::unmap_phys_adr(pg, page_size));
		if (is_fail(r)) {
			log()(SRCPOS)("() failed page free:").u(r)
			     (" page:")(pg)();
		}
	}

	page_cnt.sub(1);
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
	++alloc_cnt;

	return free_chain.remove_head();
}

/// @brief  memobj を1つ開放する。
bool mempool::page::free(const mempool& pool, memobj* obj)
{
	const void* obj_beg = memory;
	const void* obj_end = memory + pool.get_total_obj_size();

	if (!(obj_beg <= obj && obj < obj_end))
		return false;

	--alloc_cnt;

	free_chain.insert_head(obj);

	return true;
}

void mempool::page::dump(output_buffer& lt)
{
	lt("memory:")(memory)(", alloc_cnt:").u(alloc_cnt)();

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

void mempool::mp_mem_allocator::init(mempool* _mp)
{
	ops = global_vars::core.mempool_ctl_obj->get_mp_allocator_ops();
	mp = _mp;
}

cause::pair<void*> mempool::mp_mem_allocator::on_mem_allocator_Allocate(
    uptr bytes)
{
	if (CONFIG_DEBUG_VALIDATE >= 1) {
		if (mp->obj_size < bytes) {
			log()(SRCPOS)(" !!!").
			    u(bytes)("(required size) > ").
			    u(mp->obj_size)("(object size)");
			return null_pair(cause::BADARG);
		}
	}

	return mp->acquire();
}

cause::t mempool::mp_mem_allocator::on_mem_allocator_Deallocate(
    void* p)
{
	return mp->release(p);
}


void* operator new (uptr size, mempool* mp)
{
#if CONFIG_DEBUG_VALIDATE >= 1
	if (size > mp->get_obj_size()) {
		log()(SRCPOS)
		    ("!!!!new object size is over the mempool object size.")
		    ("\nnew object size:").u(size)
		    (", mempool object size:").u(mp->get_obj_size())();
		return nullptr;
	}
#endif  // CONFIG_DEBUG_VALIDATE

	auto r = mp->acquire();
	if (is_ok(r))
		return r.get_value();
	else
		return nullptr;
}

void* operator new [] (uptr size, mempool* mp)
{
#if CONFIG_DEBUG_VALIDATE >= 1
	if (size > mp->get_obj_size()) {
		log()(SRCPOS)
		    ("!!!!new object size is over the mempool object size.")
		    ("\nnew object size:").u(size)
		    (", mempool object size:").u(mp->get_obj_size())();
		return nullptr;
	}
#endif  // CONFIG_DEBUG_VALIDATE

	auto r = mp->acquire();
	if (is_ok(r))
		return r.get_value();
	else
		return nullptr;
}
