/// @file  mempool.cc
/// @brief Memory pooler.
//
// (C) 2011 KATO Takeshi
//

#include "mempool.hh"
#include "mempool_ctl.hh"

#include "global_vars.hh"
#include "log.hh"
#include "placement_new.hh"


mempool::mempool(u32 _obj_size, arch::page::TYPE ptype)
:   obj_size(normalize_obj_size(_obj_size)),
    page_type(ptype == arch::page::INVALID ? auto_page_type(obj_size) : ptype),
    alloc_count(0),
    page_count(0),
    freeobj_count(0)
{
	page_size =
	    arch::page::size_of_type(page_type);

	page_objs = (page_size - sizeof (page)) / obj_size;

	total_obj_size = page_objs * obj_size;
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

	void* p = arch::map_phys_adr(padr, page_size);

	page* pg = new (p) page;
	pg->init_onpage(*this);

	++page_count;

	return pg;
}

void mempool::delete_page(page* pg)
{
	operator delete(pg, pg);

	const cause::stype r =
	    arch::page::free(page_type, arch::unmap_phys_adr(pg, page_size));
	if (is_fail(r)) {
		log()(__func__)("() failed page free:").u(r)(" page:")(pg)();
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

	log()(__func__)("() no matched page: obj:")(obj)();
}


// mempool::page


/// @brief initialize as onpage.
void mempool::page::init_onpage(const mempool& parent)
{
	memory = onpage_get_memory();

	const u32 objsize = parent.get_obj_size();

	for (int i = parent.get_page_objs() - 1; i >= 0; --i) {
		void* obj = &memory[objsize * i];
		free_chain.insert_head(new (obj) memobj);
	}
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



cause::stype mempool_ctl::init()
{
	mempool tmp_mp(sizeof (mempool), arch::page::L1);
	mempool::page* pg = tmp_mp.new_page();

	own_mempool = new (pg->alloc())
	    mempool(sizeof (mempool), arch::page::L1);

	own_mempool->attach(pg);

	return cause::OK;
}

mempool* mempool_ctl::shared_mempool(u32 objsize)
{
	objsize = mempool::normalize_obj_size(objsize);

	mempool* r = find_shared(objsize);

	if (!r)
		r = create_shared(objsize);

	return r;
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
	mempool* r = new (own_mempool->alloc()) mempool(objsize);
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

mempool* mempool_create_shared(u32 objsize)
{
	return global_vars::gv.mempool_ctl_obj->shared_mempool(objsize);
}

