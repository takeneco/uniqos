/// @file  mem_pool.cc
/// @brief Memory pooler.
//
// (C) 2011 KATO Takeshi
//

#include "memcache.hh"
#include "mempool_ctl.hh"
#include "global_vars.hh"
#include "placement_new.hh"


mem_pool::mem_pool(u32 _obj_size, arch::page::TYPE ptype)
{
	obj_size = normalize_obj_size(_obj_size);

	page_type = 
	    ptype == arch::page::INVALID ? auto_page_type(obj_size) : ptype;

	page_size =
	    arch::page::inline_page_size(page_type);

	page_objs = (page_size - sizeof (page)) / obj_size;
}

void* mem_pool::alloc()
{
	memobj* obj = free_objs.remove_head();
	if (obj)
		return obj;

	page* pg = partial_pages.head();
	if (pg == 0) {
		// free_pages から partial_pages へ移動
		pg = free_pages.remove_head();
		if (pg == 0) {
			pg = new_page();
			if (pg == 0)
				return 0;
		}
		partial_pages.insert_head(pg);
	}

	obj = pg->alloc();

	if (pg->is_full()) {
		partial_pages.remove_head(); // remove pg
		full_pages.insert_head(pg);
	}

	return obj;
}

void mem_pool::free(void* ptr)
{
	page* pg = new (ptr) page;

	free_pages.insert_head(pg);
}

arch::page::TYPE mem_pool::auto_page_type(u32 objsize)
{
	arch::page::TYPE r = arch::page::type_of_size(objsize * 4);
	if (r == arch::page::INVALID)
		r = arch::page::type_of_size(objsize);

	return r;
}

u32 mem_pool::normalize_obj_size(u32 objsize)
{
	u32 r = max<u32>(objsize, sizeof (page));

	r = up_align<u32>(r, arch::BASIC_TYPE_ALIGN);

	return r;
}

void mem_pool::attach(page* pg)
{
	if (pg->is_free())
		free_pages.insert_head(pg);
	else if (pg->is_full())
		full_pages.insert_head(pg);
	else
		partial_pages.insert_head(pg);
}

mem_pool::page* mem_pool::new_page()
{
	uptr padr;
	if (arch::page::alloc(page_type, &padr) != cause::OK)
		return 0;

	void* p = arch::map_phys_adr(padr, page_size);

	page* pg = new (p) page;
	pg->init_onpage(*this);

	return pg;
}


// mem_pool::page


/// @brief initialize as onpage.
void mem_pool::page::init_onpage(const mem_pool& parent)
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
/// 空き memobj が無い mem_pool で alloc() を呼び出してはならない。
mem_pool::memobj* mem_pool::page::alloc()
{
	++alloc_count;

	return free_chain.remove_head();
}

/// @brief  memobj を1つ開放する。
bool mem_pool::page::free(const mem_pool& parent, memobj* obj)
{
	u8* p = reinterpret_cast<u8*>(obj);

	if (p < memory)
		return false;

	if (memory + parent.get_page_objs() * parent.get_obj_size() < p)
		return false;

	--alloc_count;

	free_chain.insert_head(obj);

	return true;
}


cause::stype mempool_ctl::init()
{
	mem_pool tmp_mp(sizeof (mem_pool), arch::page::L1);
	mem_pool::page* pg = tmp_mp.new_page();

	own_mempool = new (pg->alloc())
	    mem_pool(sizeof (mem_pool), arch::page::L1);

	own_mempool->attach(pg);

	return cause::OK;
}

mem_pool* mempool_ctl::shared_mem_pool(u32 objsize)
{
	objsize = mem_pool::normalize_obj_size(objsize);

	mem_pool* r = find_shared(objsize);

	if (!r)
		r = create_shared(objsize);

	return r;
}

/// @brief  Find existing shared mem_cache.
mem_pool* mempool_ctl::find_shared(u32 objsize)
{
	for (mem_pool* mp = shared_chain.head();
	     mp;
	     mp = shared_chain.next(mp))
	{
		if (mp->get_obj_size() == objsize)
			return mp;
	}

	return 0;
}

mem_pool* mempool_ctl::create_shared(u32 objsize)
{
	mem_pool* r = new (own_mempool->alloc()) mem_pool(objsize);
	if (r == 0)
		return 0;

	for (mem_pool* mp = shared_chain.head();
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

mem_pool* mempool_create_shared(u32 objsize)
{
	return global_vars::gv.mempool_ctl_obj->shared_mem_pool(objsize);
}

