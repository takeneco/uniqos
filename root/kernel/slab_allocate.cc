/// @file  slab_allocate.cc
/// @brief slab allocater.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "global_vars.hh"
#include "placement_new.hh"
#include "slab.hh"

#include "log.hh"


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

/// @brief スラブオブジェクトを１つ開放する。
//
/// obj must not be null.
void mem_cache::slab::free(mem_cache& parent, void* obj)
{
	--alloc_count;
}


mem_cache::mem_cache(u32 _obj_size, arch::page::TYPE pt, bool force_offslab) :
	obj_size(_obj_size)
{
	slab_page_type =
	    pt == arch::page::INVALID ? auto_page_type(pt) : pt;

	slab_page_size =
	    arch::page::inline_page_size(slab_page_type);

	slab_maxobjs =
	    (slab_page_size-sizeof(slab))/(obj_size + sizeof (slab::obj_desc));
}

void* mem_cache::alloc()
{
	slab* s = partial_chain.get_head();
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
/*
	slabobj* obj = slabobj::from_mem(ptr);
	slab* s = obj->get_parent();

	const bool fulled = s->is_full();

	s->free(obj);

	if (fulled) {
		full_chain.remove(s);
		if (s->is_free()) {
			free_chain.insert_head(s);
		} else {
			partial_chain.insert_head(s);
		}
	} else {
		if (s->is_free()) {
			partial_chain.remove(s);
			free_chain.insert_head(s);
		}
	}
*/
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

	void* p = arch::map_phys_mem(padr, slab_page_size);

	slab* s = new (p) slab;
	s->init_onslab(*this);

	return s;
}


cause::stype slab_init()
{
log()("---- slab_init() start ----")();

log()("sizeof (mem_cache) = ").u(sizeof (mem_cache))();
log()("sizeof (mem_cache::slab) = ").u(sizeof (mem_cache::slab))();

	mem_cache tmp_mc(sizeof (mem_cache), arch::page::L1);
	mem_cache::slab* s = tmp_mc.new_slab();

log()("s = ")(s)();
log()("tmp_mc.get_slab_maxobjs() = ").u(tmp_mc.get_slab_maxobjs())();

	mem_cache* sc = new (s->alloc(tmp_mc))
	    mem_cache(sizeof (mem_cache), arch::page::L1);

log()("sc = ")(sc)();

log()("s->alloc(tmp_mc) = ")(s->alloc(tmp_mc))();

log()("---- slab_init() end ----")();
	return cause::OK;
}

