/// @file  slab_allocate.cc
/// @brief slab allocater.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "global_vars.hh"
#include "memcache.hh"
#include "placement_new.hh"
#include "string.hh"


// TODO: this will obsolate
namespace memory {
	void* alloc(uptr);
}

/**
 * onslab
 * スラブページの中にスラブオブジェクト(slabobj)を含む。
 * スラブ(class slab)の直後にスラブオブジェクトが続く。
 *
 * slabobj の直後にスラブのメモリがある。
 * 空き slabobj のメモリは slab::freeobj になっていて free_chain に
 * つながっている。
 */


u8* mem_cache::slab::onslab_get_obj_head(const mem_cache& parent)
{
	const uptr endindex_adr =
	    reinterpret_cast<uptr>(&ref_obj_desc(parent.get_slab_maxobjs()));

	const uptr r = up_align<uptr>(endindex_adr, arch::BASIC_TYPE_ALIGN);

	return reinterpret_cast<u8*>(r);
}

/// onslab として初期化する。
void mem_cache::slab::init_onslab(mem_cache& parent)
{
	obj_head = onslab_get_obj_head(parent);

	const objindex last_obj = parent.get_slab_maxobjs() - 1;
	for (objindex i = 0; i < last_obj; ++i)
		ref_obj_desc(i).next_free = i + 1;

	ref_obj_desc(last_obj).next_free = ENDOBJ;
}

/// @brief スラブオブジェクトを１つ確保する。
/// @return 確保したオブジェクトを返す。
/// @return 確保できなければ 0 を返す。
inline void* mem_cache::slab::alloc(mem_cache& parent)
{
	//// checked by parent mem_cache
	//if (first_free == ENDOBJ)
	//	return 0;

	objindex i = first_free;
	first_free = ref_obj_desc(first_free).next_free;

	++alloc_count;

	return obj_head + parent.get_obj_size() * i;
}

bool mem_cache::slab::back(mem_cache& parent, void* obj)
{
	u8* p = reinterpret_cast<u8*>(obj);

	if (p < obj_head)
		return false;

	if (obj_head + parent.get_slab_maxobjs() * parent.get_obj_size() < p)
		return false;

	int i = (p - obj_head) / parent.get_obj_size();
	ref_obj_desc(i).next_free = first_free;
	first_free = i;

	--alloc_count;

	return true;
}


mem_cache::mem_cache(u32 _obj_size, arch::page::TYPE pt, bool force_offslab) :
	obj_size(_obj_size),
	free_objs_avail(0)
{
	slab_page_type =
	    pt == arch::page::INVALID ? auto_page_type(_obj_size) : pt;

	slab_page_size =
	    arch::page::size_of_type(slab_page_type);

	slab_maxobjs =
	    (slab_page_size - sizeof(slab)) /
	    (obj_size + sizeof (slab::obj_desc));
}

void mem_cache::attach(slab* s)
{
	if (s->is_free())
		free_chain.insert_head(s);
	else if (s->is_full())
		full_chain.insert_head(s);
	else
		partial_chain.insert_head(s);
}

void* mem_cache::alloc()
{
	if (free_objs_avail != 0)
		return free_objs[--free_objs_avail];

	slab* s = partial_chain.head();
	if (s == 0) {
		// free_chain から partial_chain へ移動
		s = free_chain.remove_head();
		if (s == 0) {
			s = new_slab();
			if (s == 0)
				return 0;
		}
		partial_chain.insert_head(s);
	}

	void* obj = s->alloc(*this);

	if (s->is_full()) {
		partial_chain.remove_head(); // remove s
		full_chain.insert_head(s);
	}

	return obj;
}

void mem_cache::free(void* ptr)
{
	if (free_objs_avail >= FREE_OBJS_LEN) {
		back_slab();
	}

	free_objs[free_objs_avail++] = ptr;
}

arch::page::TYPE mem_cache::auto_page_type(u32 objsize)
{
	arch::page::TYPE r = arch::page::type_of_size(objsize * 4);
	if (r == arch::page::INVALID)
		r = arch::page::type_of_size(objsize);

	return r;
}

mem_cache::slab* mem_cache::new_slab()
{
	uptr padr;
	if (arch::page::alloc(slab_page_type, &padr) != cause::OK)
		return 0;

	void* p = arch::map_phys_adr(padr, slab_page_size);

	slab* s = new (p) slab;
	s->init_onslab(*this);

	return s;
}

/// free_obj に溜まった slabobj を slab に戻す。
void mem_cache::back_slab()
{
	for (int i = 0; i < FREE_OBJS_LEN / 2; ++i) {
		void* p = free_objs[i];

		slab* s;
		for (s = partial_chain.head(); s; s = partial_chain.next(s)) {
			if (s->back(*this, p)) {
				if (s->is_free()) {
					partial_chain.remove(s);
					free_chain.insert_head(s);
				}
				break;
			}
		}
		if (s != 0)
			continue;
		for (s = full_chain.head(); s; s = full_chain.next(s)) {
			if (s->back(*this, p)) {
				full_chain.remove(s);
				partial_chain.insert_head(s);
				break;
			}
		}
		if (s == 0) {
			// ここに到達したら異常
log()("abnormaly.")();
		}
	}

	mem_move(
	    FREE_OBJS_LEN / 2 * sizeof free_objs[0],
	    &free_objs[FREE_OBJS_LEN / 2],
	    &free_objs[0]);
	free_objs_avail = FREE_OBJS_LEN / 2;
}


class memcache_control
{
	typedef bichain<mem_cache, &mem_cache::chain_hook> memcache_chain;

	mem_cache* own_memcache;
	memcache_chain shared_chain;

public:
	memcache_control() {}
	cause::stype init();

	mem_cache* shared_mem_cache(u32 obj_size);

private:
	mem_cache* shared_find(u32 obj_size);
	mem_cache* shared_new(u32 obj_size);
};


cause::stype memcache_control::init()
{
	mem_cache tmp_mc(sizeof (mem_cache), arch::page::L1);
	mem_cache::slab* s = tmp_mc.new_slab();

	own_memcache = new (s->alloc(tmp_mc))
	    mem_cache(sizeof (mem_cache), arch::page::L1);

	own_memcache->attach(s);

	return cause::OK;
}

mem_cache* memcache_control::shared_mem_cache(u32 obj_size)
{
	obj_size = up_align<u32>(obj_size, arch::BASIC_TYPE_ALIGN);

	mem_cache* r = shared_find(obj_size);
	if (r == 0)
		r = new (own_memcache->alloc()) mem_cache(obj_size);

	return r;
}

/// @brief  find existing shared mem_cache.
mem_cache* memcache_control::shared_find(u32 obj_size)
{
	for (mem_cache* mc = shared_chain.head();
	     mc;
	     mc = shared_chain.next(mc))
	{
		if (mc->get_obj_size() == obj_size)
			return mc;
	}

	return 0;
}

/// @brief  create new shared mem_cache.
mem_cache* memcache_control::shared_new(u32 obj_size)
{
	mem_cache* newmc = new (own_memcache->alloc()) mem_cache(obj_size);
	if (newmc == 0)
		return 0;

	for (mem_cache* mc = shared_chain.head();
	     mc;
	     mc = shared_chain.next(mc))
	{
		if (obj_size < mc->get_obj_size())
			shared_chain.insert_prev(mc, newmc);
	}

	return newmc;
}


cause::stype slab_init()
{
	global_vars::gv.memcache_ctl =
	    new (memory::alloc(sizeof (memcache_control))) memcache_control;

	global_vars::gv.memcache_ctl->init();

	return cause::OK;
}

mem_cache* shared_mem_cache(u32 obj_size)
{
	return global_vars::gv.memcache_ctl->shared_mem_cache(obj_size);
}


void mem_cache::slab::dump(log_target& log)
{
	log("first_free = ").u(first_free)();
	log("alloc_count = ").u(alloc_count)();
	log("obj_head = ")(obj_head)();
}

void mem_cache::dump(log_target& log)
{
	log("free_objs_avail = ").u(free_objs_avail)();
	log("free_objs = ")(free_objs[0])(", ")(free_objs[1])(", ")
	(free_objs[2])(", ")(free_objs[3])();
	log("------free_chain:")();
	for (slab* s = free_chain.head(); s; s = free_chain.next(s)) {
		s->dump(log);
		log("------")();
	}
	log("------partial_chain:")();
	for (slab* s = partial_chain.head(); s; s = partial_chain.next(s)) {
		s->dump(log);
		log("------")();
	}
	log("------full_chain:")();
	for (slab* s = full_chain.head(); s; s = full_chain.next(s)) {
		s->dump(log);
		log("------")();
	}
}
