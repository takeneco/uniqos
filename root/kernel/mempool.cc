/// @file  mempool.cc
/// @brief Memory pooler.
//
// (C) 2011 KATO Takeshi
//

#include "mempool_ctl.hh"

#include "global_vars.hh"
#include "log.hh"
#include "placement_new.hh"


void* mem_alloc(u32 bytes)
{
	return global_vars::gv.mempool_ctl_obj->shared_alloc(bytes);
}


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
	memobj* obj = free_objs.remove_head();
	if (obj) {
		--freeobj_count;
	} else {
		page* pg = free_pages.head();
		if (pg == 0) {
			pg = new_page();
			if (pg == 0)
				return 0;

			free_pages.insert_head(pg);
		}

		obj = pg->alloc();

		if (pg->is_full()) {
			free_pages.remove_head(); // remove pg
			full_pages.insert_head(pg);
		}
	}

	++alloc_count;

	return obj;
}

void mempool::free(void* ptr)
{
	memobj* obj = new (ptr) memobj;

	free_objs.insert_head(obj);

	--alloc_count;
	++freeobj_count;
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

	r = up_align<u32>(r, arch::BASIC_TYPE_ALIGN);

	return r;
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
	uptr padr;
	if (arch::page::alloc(page_type, &padr) != cause::OK)
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
		page_pool->free(pg);
		arch::page::free(page_type, adr);
	} else {
		const cause::stype r = arch::page::free(
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


// mempool_ctl


cause::stype mempool_ctl::init()
{
	mempool tmp_mp(sizeof (mempool), arch::page::L1, 0);
	mempool::page* pg = tmp_mp.new_page();
	if (UNLIKELY(!pg))
		return cause::NO_MEMORY;

	own_mempool = new (pg->alloc())
	    mempool(sizeof (mempool), arch::page::L1);
	if (UNLIKELY(!own_mempool))
		return cause::NO_MEMORY;

	own_mempool->attach(pg);

	offpage_pool = shared_mempool(sizeof (mempool::page));
	if (UNLIKELY(!offpage_pool))
		return cause::NO_MEMORY;

	cause::stype r = init_heap();
	if (is_fail(r))
		return r;

	return cause::OK;
}

mempool* mempool_ctl::shared_mempool(u32 objsize)
{
	objsize = mempool::normalize_obj_size(objsize);

	mempool* r = find_shared(objsize);

	if (!r)
		r = create_shared(objsize);

	r->inc_shared_count();

	return r;
}

void mempool_ctl::release_shared_mempool(mempool* mp)
{
	if (mp->dec_shared_count() == 0 && mp->get_alloc_count() == 0) 
		mp->destroy();
}

mempool* mempool_ctl::exclusived_mempool(
    u32 objsize,
    arch::page::TYPE page_type,
    PAGE_STYLE page_style)
{
	decide_params(&objsize, &page_type, &page_style);

	mempool* pgpl = page_style == ONPAGE ? 0 : offpage_pool;

	mempool* r =
	    new (own_mempool->alloc()) mempool(objsize, page_type, pgpl);

	return r;
}

namespace {

struct heap
{
	mempool* mp;
	u8 mem[];
};

}

void* mempool_ctl::shared_alloc(u32 bytes)
{
	const u32 size = mempool::normalize_obj_size(bytes + sizeof (mempool*));

	mempool* pool = 0;
	for (mempool* mp = shared_chain.head();
	     mp;
	     mp = shared_chain.next(mp))
	{
		if (mp->get_obj_size() >= size) {
			pool = mp;
			break;
		}
	}

	if (!pool)
		return 0;

	heap* h = static_cast<heap*>(pool->alloc());
	if (!h)
		return 0;

	h->mp = pool;

	return h->mem;
}

void mempool_ctl::dump(log_target& lt)
{
	lt("obj_size      alloc_count       page_count    freeobj_count")();
	for (mempool* mp = shared_chain.head();
	     mp;
	     mp = shared_chain.next(mp))
	{
		mp->dump(lt);
	}
}

cause::stype mempool_ctl::init_heap()
{
	const uptr sizes[] = {
		0x1000, 0x2000, 0x3000, 0x4000, 0x200000,
	};

	for (uptr i = 0; i < sizeof sizes / sizeof sizes[0]; ++i) {
		mempool* pool = create_shared(sizes[i]);
		if (!pool)
			return cause::FAIL;
	}

	return cause::OK;
}

/// @brief  Find existing shared mem_cache.
mempool* mempool_ctl::find_shared(u32 objsize)
{
	for (mempool* mp = shared_chain.head();
	     mp;
	     mp = shared_chain.next(mp))
	{
		if (mp->get_obj_size() == objsize)
			return mp;
	}

	return 0;
}

mempool* mempool_ctl::create_shared(u32 objsize)
{
	arch::page::TYPE page_type = arch::page::INVALID;
	PAGE_STYLE page_style = ENTRUST;

	decide_params(&objsize, &page_type, &page_style);

	mempool* pgpl = page_style == ONPAGE ? 0 : offpage_pool;

	mempool* r =
	    new (own_mempool->alloc()) mempool(objsize, page_type, pgpl);
	if (r == 0)
		return 0;

	for (mempool* mp = shared_chain.head();
	     mp;
	     mp = shared_chain.next(mp))
	{
		if (objsize < mp->get_obj_size()) {
			shared_chain.insert_prev(mp, r);
			return r;
		}
	}

	shared_chain.insert_tail(r);
	return r;
}

void mempool_ctl::decide_params(
    u32*              objsize,
    arch::page::TYPE* page_type,
    PAGE_STYLE*       page_style)
{
	*objsize = mempool::normalize_obj_size(*objsize);

	if (*page_type == arch::page::INVALID)
		*page_type = arch::page::type_of_size(*objsize * 8);

	if (*page_style == ENTRUST) {
		if (*objsize < 512) {
			*page_style = ONPAGE;
		} else {
			uptr page_size = arch::page::size_of_type(*page_type);
			if ((page_size % *objsize) < sizeof (mempool::page))
				*page_style = OFFPAGE;
			else
				*page_style = ONPAGE;
		}
	}
}

mempool* mempool_create_shared(u32 objsize)
{
	return global_vars::gv.mempool_ctl_obj->shared_mempool(objsize);
}

