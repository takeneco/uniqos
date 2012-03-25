/// @file  mempool_ctl.cc
/// @brief Memory pool controller.

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

#include <global_vars.hh>
#include <log.hh>
#include <placement_new.hh>

#include <processor.hh>


void* mem_alloc(u32 bytes)
{
	return global_vars::gv.mempool_ctl_obj->shared_alloc(bytes);
}


// mempool_ctl


cause::stype mempool_ctl::init()
{
	mempool tmp_mp(sizeof (mempool), arch::page::L1, 0);
	mempool::page* pg = tmp_mp.new_page();
	if (UNLIKELY(!pg))
		return cause::NOMEM;

	own_mempool = new (pg->alloc())
	    mempool(sizeof (mempool), arch::page::L1);
	if (UNLIKELY(!own_mempool))
		return cause::NOMEM;

	own_mempool->attach(pg);

	offpage_pool = shared_mempool(sizeof (mempool::page));
	if (UNLIKELY(!offpage_pool))
		return cause::NOMEM;

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

}  // namespace

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

extern "C" mempool* mempool_create_shared(u32 objsize)
{
	return global_vars::gv.mempool_ctl_obj->shared_mempool(objsize);
}

extern "C" void mempool_release_shared(mempool* mp)
{
	global_vars::gv.mempool_ctl_obj->release_shared_mempool(mp);
}


cause::stype page_alloc(arch::page::TYPE page_type, uptr* padr)
{
	processor* proc = get_current_cpu();

	proc->preempt_disable();

	cause::stype r = arch::page::alloc(page_type, padr);

	proc->preempt_enable();

	return r;
}

cause::stype page_dealloc(arch::page::TYPE page_type, uptr padr)
{
	processor* proc = get_current_cpu();

	proc->preempt_disable();

	cause::stype r = arch::page::free(page_type, padr);

	proc->preempt_enable();

	return r;
}

