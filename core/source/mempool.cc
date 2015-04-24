/// @file  mempool.cc
/// @brief Memory pooler.

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

#include "mempool_ctl.hh"

#include <core/cpu_node.hh>
#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/page.hh>
#include <util/string.hh>


// mempool::page

/// mempool::page は thread safe ではないので呼び出し元の mempool::node で
/// ロック制御する必要がある。

mempool::page::page() :
	acquire_cnt(0)
{
}

bool mempool::page::is_full() const
{
	return free_chain.is_empty();
}

bool mempool::page::is_free() const
{
	return acquire_cnt == 0;
}

u8* mempool::page::get_memory()
{
	return memory;
}

/// @brief Initialize as onpage.
void mempool::page::init_as_onpage(const mempool& pool)
{
	memory = onpage_get_memory();

	init(pool);
}

/// @brief Initialize as offpage.
void mempool::page::init_as_offpage(const mempool& pool, void* _memory)
{
	memory = reinterpret_cast<u8*>(_memory);

	init(pool);
}

/// @brief  memobj を1つ確保する。
//
/// 空き memobj が無い mempool::page で acquire() を呼び出してはならない。
mempool::memobj* mempool::page::acquire()
{
	++acquire_cnt;

	return free_chain.pop_front();
}

/// @brief  memobj を1つ開放する。
bool mempool::page::release(const mempool& pool, memobj* obj)
{
	const void* page_head = memory;
	const void* page_tail = memory + pool.get_page_size();

	if (!(page_head <= obj && obj < page_tail))
		return false;

	--acquire_cnt;

	free_chain.push_front(obj);

	return true;
}

void mempool::page::dump(output_buffer& lt)
{
	lt("memory:")(memory)(", acquire_cnt:").u(acquire_cnt)();

	lt("{");

	for (auto obj : free_chain)
		lt(obj)(",");

	lt("}")();
}

void mempool::page::init(const mempool& owner)
{
	const u32 objsize = owner.get_obj_size();
	const u32 objcnt = owner.get_page_objs();

	for (u32 i = 0; i < objcnt; ++i) {
		void* obj = &memory[objsize * i];
		free_chain.push_front(new (obj) memobj);
	}
}


// mempool::node

mempool::node::node() :
	freeobj_cnt(0)
{
}

/// memobj を１つ確保する。
/// @return 確保した memobj のメモリへのポインタを返す。
///         memobj の確保に失敗した場合は nullptr を返す。
//
/// 失敗した場合は、push_page() で新しい page を追加してから呼び直せば、
/// 新しいページから memobj が確保される。
void* mempool::node::acquire()
{
	spin_lock_section _sls(lock);

	memobj* obj = free_objs.pop_front();
	if (obj) {
		--freeobj_cnt;
	} else {
		page* pg = free_pages.front();
		if (pg == 0)
			return 0;

		obj = pg->acquire();

		if (pg->is_full()) {
			free_pages.pop_front(); // == pg
			full_pages.push_front(pg);
		}
	}

	return obj;
}

/// memobj を解放する。
void mempool::node::release(void* ptr)
{
	memobj* obj = new (ptr) memobj;

	spin_lock_section _sls(lock);

	free_objs.push_front(obj);

	++freeobj_cnt;
}

void mempool::node::push_page(page* new_page)
{
	lock.lock();

	free_pages.push_front(new_page);

	lock.unlock();
}

/// free_objs から memobj を１つ解放する。
/// @return free_objs が空か、memobj の解放に成功すると nullptr を返す。
///         memobj の解放に失敗すると失敗した memobj へのポインタを返す。
///         失敗したときは memobj が他の node に属している。
//
/// 失敗した場合に返される memobj は free_objs から外されている。
mempool::memobj* mempool::node::pop_freeobj(mempool* owner)
{
	lock.lock();

	memobj* obj = free_objs.pop_front();

	// back_to_page() は独自にロックしているので、ここで unlock する。
	lock.unlock();

	if (obj == nullptr || back_to_page(obj, owner))
		return nullptr;
	else
		return obj;
}

void mempool::node::collect_free_pages(mempool* owner)
{
	const sptr SAVE_FREEOBJS = 64;

	const sptr n = freeobj_cnt - SAVE_FREEOBJS;
	for (sptr i = 0; i < n; ++i) {
		memobj* obj = free_objs.pop_front();
		if (back_to_page(obj, owner))
			--freeobj_cnt;
		else
			free_objs.pop_front();
	}
}

/// mempool::page から確保した memobj を元の mempool::page へ戻す。
/// @param[in] owner  ページのサイズ情報へのアクセスとページの解放に使用する
/// @retval true   obj の解放に成功した。
/// @retval false  obj の解放に失敗した。obj は別のnodeに属している。
bool mempool::node::back_to_page(memobj* obj, mempool* owner)
{
	spin_lock_section _sls(lock);

	for (page* pg = free_pages.front(); pg; pg = free_pages.next(pg)) {
		if (pg->release(*owner, obj)) {
			if (pg->is_free()) {
				free_pages.remove(pg);
				owner->delete_page(pg);
			}
			return true;
		}
	}

	for (page* pg = full_pages.front(); pg; pg = full_pages.next(pg)) {
		if (pg->release(*owner, obj)) {
			full_pages.remove(pg);
			free_pages.push_front(pg);
			return true;
		}
	}

	return false;
}

void mempool::node::import_dirty_page(page* page)
{
	lock.lock();

	if (page->is_full())
		free_pages.push_front(page);
	else
		full_pages.push_front(page);

	lock.unlock();
}


// mempool

mempool::mempool(u32 _obj_size, arch::page::TYPE ptype, mempool* _page_pool) :
	obj_size(normalize_obj_size(_obj_size)),
	page_type(ptype == arch::page::INVALID ?
	          auto_page_type(obj_size) : ptype),
	page_size(arch::page::size_of_type(page_type)),
	page_objs((page_size - sizeof (page)) / obj_size),
	alloc_cnt(0),
	page_cnt(0),
	shared_count(0),
	page_pool(_page_pool)
{
	obj_name[0] = '\0';

	for (cpu_id_t i = 0; i < CONFIG_MAX_CPUS; ++i)
		mempool_nodes[i] = nullptr;
}

void mempool::setup_mem_allocator(const mem_allocator::interfaces* ifs)
{
	_mem_allocator.init(ifs, this);
}

cause::t mempool::destroy()
{
	if (alloc_cnt.load() != 0 ||
	    page_cnt.load() != 0 ||
	    shared_count.load() != 0)
	{
		log()("FAULT:")
		(SRCPOS)("(): Bad operation.")()
		("| alloc_cnt: ").u(alloc_cnt.load())
		(", page_cnt: ").u(page_cnt.load())()
		(", shared_count: ").u(shared_count.load())();

		return cause::FAIL;
	}

	if (collect_all_freeobjs())
		return cause::FAIL;

	return cause::OK;
}

cause::pair<mempool*> mempool::create_exclusive(
    u32 objsize,
    arch::page::TYPE page_type,
    PAGE_STYLE page_style)
{
	return global_vars::core.mempool_ctl_obj->create_exclusived_mp(
	    objsize, page_type, page_style);
}

/// @retval FAIL  未解放のメモリがある。
cause::t mempool::destroy_exclusive(mempool* mp)
{
	return global_vars::core.mempool_ctl_obj->destroy_exclusived_mp(mp);
}

cause::pair<mempool*> mempool::acquire_shared(u32 objsize)
{
	mempool_ctl* mpctl = global_vars::core.mempool_ctl_obj;

	return mpctl->acquire_shared_mempool(objsize);
}

void mempool::release_shared(mempool* mp)
{
	global_vars::core.mempool_ctl_obj->release_shared_mempool(mp);
}

cause::pair<void*> mempool::acquire()
{
	preempt_disable_section _pds;

	const cpu_id_t cpuid = arch::get_cpu_node_id();

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

	const cpu_id cpuid = arch::get_cpu_node_id();

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
	preempt_disable_section _pds;

	const cpu_id cpuid = arch::get_cpu_node_id();

	mempool_nodes[cpuid]->collect_free_pages(this);
}

void mempool::collect_one_freeobj()
{
	cpu_id cur_cpuid = arch::get_cpu_node_id();

	memobj* obj = mempool_nodes[cur_cpuid]->pop_freeobj(this);

	if (obj)
		back_to_page(cur_cpuid, obj);
}

/// freeobj をすべて回収する。
/// @retval true   freeobj をすべて回収した。
/// @retval false  回収されなかった freeobj がある。
//
/// この関数の実行中に freeobj が増えると戻り値が false になる場合がある。
bool mempool::collect_all_freeobjs()
{
	const cpu_id cpu_num = get_cpu_node_count();

	for (cpu_id cpu = 0; cpu < cpu_num; ++cpu) {

		sptr objcnt = mempool_nodes[cpu]->get_freeobj_cnt();
		for (sptr i = 0; i < objcnt; ++i) {

			memobj* obj = mempool_nodes[cpu]->pop_freeobj(this);
			if (obj)
				back_to_page(cpu, obj);
		}
	}

	for (cpu_id cpu = 0; cpu < cpu_num; ++cpu) {
		if (0 != mempool_nodes[cpu]->get_freeobj_cnt())
			return false;
	}

	return true;
}

void mempool::set_obj_name(const char* name)
{
	str_copy(name, obj_name, sizeof obj_name - 1);

	obj_name[sizeof obj_name - 1] = '\0';
}

void mempool::dump(output_buffer& ob, uint level)
{
	if (level >= 1) {
		ob("obj_size    : ").u(obj_size, 12)
		("\npage_type   : ").u(page_type, 12)
		(" |page_size      : ").u(page_size, 12)
		("\npage_objs   : ").u(page_objs, 12)
		("\nalloc_cnt   : ").s(alloc_cnt.load(), 12)
		(" |page_cnt       : ").s(page_cnt.load(), 12)();
	}

	if (level >= 2) {
		for (int i = 0; i < CONFIG_MAX_CPUS; ++i) {
			node* nd = mempool_nodes[i];
			ob("nd[").u(i)("]=")(nd)();
		}
	}
}

void mempool::dump_table(output_buffer& ob)
{
	ob.str(obj_name, 14)(' ').
	   u(obj_size, 11)(' ').
	   u(alloc_cnt.load(), 11)(' ').
	   u(page_cnt.load(), 11)(' ')();
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

	void* r = nd->acquire();
	if (!r) {
		page* pg = new_page(cpuid);
		if (UNLIKELY(!pg))
			return 0;

		nd->push_page(pg);

		r = nd->acquire();

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
	const int cpuid = arch::get_cpu_node_id();

	mempool_nodes[cpuid]->release(ptr);

	alloc_cnt.sub(1);
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

		pg->init_as_offpage(*this, mem);
	} else {
		pg = new (mem) page;
		pg->init_as_onpage(*this);
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

void mempool::back_to_page(cpu_id src_cpuid, memobj* obj)
{
	const cpu_id last_cpu = get_cpu_node_count() - 1;
	cpu_id cpuid = src_cpuid;
	for (;;) {
		if (cpuid > 0)
			--cpuid;
		else
			cpuid = last_cpu;

		if (cpuid == src_cpuid) {
			log()(SRCPOS)
			    ("(): !!!freeobj source not found.");
			break;
		}

		if (mempool_nodes[cpuid]->back_to_page(obj, this))
			break;
	}
}

void mempool::set_node(int i, node* nd)
{
	mempool_nodes[i] = nd;
}

auto mempool::get_node(int i) -> node*
{
	return mempool_nodes[i];
}

// mempool::mp_mem_allocator

void mempool::mp_mem_allocator::init(
    const mem_allocator::interfaces* _ifs,
    mempool* _mp)
{
	ifs = _ifs;
	mp = _mp;
}

cause::pair<void*> mempool::mp_mem_allocator::on_Allocate(uptr bytes)
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

cause::t mempool::mp_mem_allocator::on_Deallocate(void* p)
{
	return mp->release(p);
}

cause::pair<uptr> mempool::mp_mem_allocator::on_GetSize(void*)
{
	return cause::make_pair<uptr>(cause::OK, mp->get_obj_size());
}

