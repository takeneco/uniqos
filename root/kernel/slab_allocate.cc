/// @file  slab_allocate.cc
/// @brief slab allocater.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "global_vars.hh"
#include "placement_new.hh"
#include "slab.hh"
#include "string.hh"


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
	    pt == arch::page::INVALID ? auto_page_type(pt) : pt;

	slab_page_size =
	    arch::page::inline_page_size(slab_page_type);

	slab_maxobjs =
	    (slab_page_size-sizeof(slab))/(obj_size + sizeof (slab::obj_desc));
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
	if (free_objs_avail >= FREE_OBJS_LEN) {
		back_slab();
	}

	free_objs[free_objs_avail++] = ptr;
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

/// free_obj に溜まった slabobj を slab に戻す。
void mem_cache::back_slab()
{
	for (int i = 0; i < FREE_OBJS_LEN / 2; ++i) {
		void* p = free_objs[i];

		slab* s;
		for (s = partial_chain.get_head();
		     s;
		     s = partial_chain.get_next(s))
		{
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
		for (s = full_chain.get_head();
		     s;
		     s = full_chain.get_next(s))
		{
			if (s->back(*this, p)) {
				full_chain.remove(s);
				partial_chain.insert_head(s);
				break;
			}
		}
		if (s == 0) {
			// ここに到達したら異常
log()("abnormal.")();
		}
	}

	memory_move(
	    &free_objs[FREE_OBJS_LEN / 2],
	    &free_objs[0],
	    FREE_OBJS_LEN / 2 * sizeof free_objs[0]);
	free_objs_avail = FREE_OBJS_LEN / 2;
}


cause::stype slab_init()
{
log()("---- slab_init() start ----")();

	mem_cache tmp_mc(sizeof (mem_cache), arch::page::L1);
	mem_cache::slab* s = tmp_mc.new_slab();

	mem_cache* sc = new (s->alloc(tmp_mc))
	    mem_cache(sizeof (mem_cache), arch::page::L1);

	sc->attach(s);

	void* buf[100];
	for (u32 i = 0; i < 11; ++i) {
		buf[i] = sc->alloc();
		log()("buf[").u(i)("] = ")(buf[i])();
	}

sc->dump(log());

	for (u32 i = 0; i < 11; ++i) {
		sc->free(buf[i]);
	}

sc->dump(log());

	for (u32 i = 0; i < 11; ++i) {
		buf[i] = sc->alloc();
		log()("buf[").u(i)("] = ")(buf[i])();
	}

sc->dump(log());

log()("---- slab_init() end ----")();
	return cause::OK;
}

void mem_cache::slab::dump(kernel_log& log)
{
	log("first_free = ").u(first_free)();
	log("alloc_count = ").u(alloc_count)();
	log("obj_head = ")(obj_head)();
}

void mem_cache::dump(kernel_log& log)
{
	log("free_objs_avail = ").u(free_objs_avail)();
	log("free_objs = ")(free_objs[0])(", ")(free_objs[1])(", ")
	(free_objs[2])(", ")(free_objs[3])();
	log("------free_chain:")();
	for (slab* s = free_chain.get_head(); s;
	    s = free_chain.get_next(s)) {
		s->dump(log);
		log("------")();
	}
	log("------partial_chain:")();
	for (slab* s = partial_chain.get_head(); s;
	    s = partial_chain.get_next(s)) {
		s->dump(log);
		log("------")();
	}
	log("------full_chain:")();
	for (slab* s = full_chain.get_head(); s;
	    s = full_chain.get_next(s)) {
		s->dump(log);
		log("------")();
	}
}
