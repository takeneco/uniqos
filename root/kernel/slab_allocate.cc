/// @file  slab_allocate.cc
/// @brief slab allocater.
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "global_variables.hh"
#include "slab.hh"


/**
 * onslab
 * スラブページの中にスラブオブジェクト(slabobj)を含む。
 * スラブ(class slab)の直後にスラブオブジェクトが続く。
 *
 * slabobj の直後にスラブのメモリがある。
 * 空き slabobj のメモリは slab::freeobj になっていて free_chain に
 * つながっている。
 */

namespace {

class slab
{
	class freeobj;

	class slabobj
	{
		slab* parent_slab;
	public:
		slabobj(slab* parent) : parent_slab(parent) {}

		slab* get_parent() {
			return parent_slab;
		}
		freeobj* get_freeobj() {
			return reinterpret_cast<freeobj*>(this + 1);
		}
	};
	class freeobj
	{
		chain_link<freeobj> chain_link_;
	public:
		chain_link<freeobj>& chain_hook() { return chain_link_; }
	};

	u8* get_onslab_obj_head() {
		return reinterpret_cast<u8*>(this + 1);
	}

	chain<freeobj, &freeobj::chain_hook> free_chain;

	bichain_link<slab> chain_link_;
	int alloc_count;

public:
	bichain_link<slab>& chain_hook() { return chain_link_; }

	slab() :
		slab_index_chain(),
		chain_link_(),
		alloc_count(0),
	{}

	void init_onslab(int objs, uptr slab_size);
	bool is_full() const { return slab_index_chain.get_head() != 0; }
	void* alloc();
	void free(void* obj);
};

/// ページ内にオブジェクトを含むスラブとして初期化する。
/// @param[in] obj_size   オブジェクトのサイズ
/// @param[in] slab_size  ページサイズ
void slab::init_onslab(uptr obj_size, uptr slab_size)
{
	obj_size = up_align(obj_size, arch::BASIC_TYPE_ALIGN)

	if (obj_size < sizeof (freeobj))
		obj_size = sizeof (freeobj);

	const int obj_count = (slab_size - sizeof *this) / obj_size;

	u8* si = get_onslab_obj_head();
	for (int i = 0; i < obj_count; ++i) {
		freeobj* obj = new (si) freeobj;
		free_chain.insert_head(obj);
		si += obj_size;
	}
}

/// @brief スラブオブジェクトを１つ確保する。
/// @return 確保したオブジェクトを返す。
/// @return 確保できなければ 0 を返す。
void* slab::alloc()
{
	void* r = free_chain.remove_head();
	if (r != 0)
		++alloc_count;

	return r;
}

/// @brief スラブオブジェクトを１つ開放する。
//
/// obj must not be null.
void slab::free(void* obj)
{
	free_chain.insert_head(new (obj) freeobj);

	--alloc_count;
}

/// TODO: 物理メモリを綺麗に確保できるようにしたい
void* physical_alloc(u32 page_size)
{
	uptr padr;

	if (page_size <= arch::PAGE_L1_SIZE) {
		if (arch::pmem::alloc_l1page(&padr) != cause::OK)
			return 0;
	} else if (page_size <= arch::PAGE_L2_SIE) {
		if (arch::pmem::alloc_l2page(&padr) != cause::OK)
			return 0;
	} else {
		if (arch::pmem::alloc_l3page(&padr) != cause::OK)
			return 0;
	}

	return arch::pmem::direct_map(padr);
}

}  // namespace


slab_alloc::slab_alloc(u32 obj_size_, u32 slab_size_) :
	pertial_chain(),
	free_chain(),
	full_chain(),
	obj_size(obj_size_),
	slab_size(slab_size_),
	chain_link_()
{
}

void* slab_alloc::alloc()
{
	slab* s = partial_chain.get_head();
	if (s == 0) {
		// free_chain から partial_chain へ移動
		s = free_chain.get_head();
		if (s == 0) {
			// TODO:new slab
			return 0;
		}
		partial_chain.insert_head(s);
	}

	void* r = s->alloc();

	if (s->is_full()) {
		partial_chain.remove_head(); // == s
		full_chain.insert_head(s);
	}

	return r;
}


cause::stype slab_init()
{
	uptr padr;
	cause::stype r = arch::pmem::alloc_l1page(&padr);
	if (r != cause::OK)
		return r;

	slab* s = new (arch::pmem::direct_map(padr)) slab;

	s->init_onslab(sizeof (slab_alloc), arch::PAGE_L1_SIZE);

	slab_alloc* sc = new (s->alloc())
	    slab_alloc(sizeof (slab_alloc), arch::PAGE_L1_SIZE);

	return cause::OK;
}

