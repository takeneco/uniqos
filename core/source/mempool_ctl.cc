/// @file  mempool_ctl.cc
/// @brief Memory pool controller.

//  UNIQOS  --  Unique Operating System
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
#include <core/mem_io.hh>
#include <core/new_ops.hh>


void* mem_alloc(u32 bytes)
{
	return global_vars::core.mempool_ctl_obj->shared_allocate(bytes);
}

void mem_dealloc(void* mem)
{
	return global_vars::core.mempool_ctl_obj->shared_deallocate(mem);
}


// mempool_ctl

mempool_ctl::mempool_ctl() :
	mempool_mp(nullptr),
	node_mp(nullptr),
	offpage_mp(nullptr)
{
}

cause::t mempool_ctl::setup()
{
	cause::t r = setup_mp();
	if (is_fail(r))
		return r;

	r = setup_shared_mp();
	if (is_fail(r))
		return r;

	return cause::OK;
}

void mempool_ctl::move_to(mempool_ctl* dest)
{
	dest->mempool_mp = this->mempool_mp;
	this->mempool_mp = nullptr;

	dest->node_mp = this->node_mp;
	this->node_mp = nullptr;

	dest->offpage_mp = this->offpage_mp;
	this->offpage_mp = nullptr;

	this->shared_chain.move_to(&dest->shared_chain);

	this->exclusived_chain.move_to(&dest->exclusived_chain);

	dest->after_move();
}

void mempool_ctl::after_move()
{
	_mp_allocator_ifs.init();
	_mp_allocator_ifs.Allocate =
	    mem_allocator::call_on_Allocate<mempool::mp_mem_allocator>;
	_mp_allocator_ifs.Deallocate =
	    mem_allocator::call_on_Deallocate<mempool::mp_mem_allocator>;
	_mp_allocator_ifs.GetSize =
	    mem_allocator::call_on_GetSize<mempool::mp_mem_allocator>;

	_shared_mem.init();

	mempool_mp->setup_mem_allocator(&_mp_allocator_ifs);
	node_mp->setup_mem_allocator(&_mp_allocator_ifs);

	for (mempool* mp = shared_chain.front();
	     mp;
	     mp = shared_chain.next(mp))
	{
		mp->setup_mem_allocator(&_mp_allocator_ifs);
	}
}

cause::t mempool_ctl::unsetup()
{
	cause::t r = unsetup_shared_mp();
	if (is_fail(r))
		return r;

	r = unsetup_mp();
	if (is_fail(r))
		return r;

	return cause::OK;
}


/// @brief shared mempool に名前をつける。
/// @pre mem_io が使用可能であること。
cause::t mempool_ctl::post_setup()
{
	for (mempool* mp : shared_chain) {
		const u32 obj_size = mp->get_obj_size();

		char obj_name[sizeof mp->obj_name];
		mem_io obj_name_io(obj_name);

		output_buffer(&obj_name_io, 0)
		    ("shared-").u(obj_size)
		    ('\0');

		mp->set_obj_name(obj_name);
	}

	return cause::OK;
}

cause::pair<mempool*> mempool_ctl::acquire_shared_mempool(u32 objsize)
{
	mempool* _mp = find_shared(objsize);
	if (!_mp)
		return null_pair(cause::FAIL);

	_mp->inc_shared_count();

	return make_pair(cause::OK, _mp);
}

void mempool_ctl::release_shared_mempool(mempool* mp)
{
	mp->dec_shared_count();

	// TODO:解放できていない。
	if (mp->get_shared_count() == 0 && mp->get_alloc_cnt() == 0) 
		mp->destroy();
}

/// @brief  用途限定の mempool を生成する。
//
/// pate_type に INVALID を指定すると objsize に合わせて自動で選択する。
cause::pair<mempool*> mempool_ctl::create_exclusived_mp(
    u32 objsize,                    ///< [in] オブジェクトサイズ。
    arch::page::TYPE page_type,     ///< [in] ページタイプを指定する。
    mempool::PAGE_STYLE page_style) ///< [in] ONPAGE/OFFPAGE/ENTRUSTを指定する。
{
	cause::t r = decide_params(&objsize, &page_type, &page_style);
	if (is_fail(r))
		return null_pair(r);

	mempool* opp = page_style == mempool::ONPAGE ? 0 : offpage_mp;

	//mempool* new_mp = new (*mempool_mp) mempool(objsize, page_type, opp);
	mempool* new_mp = new (mempool_mp->alloc()) mempool(objsize, page_type, opp);
	if (!new_mp)
		return null_pair(cause::NOMEM);

	const cpu_id cpu_num = get_cpu_node_count();
	for (cpu_id i = 0; i < cpu_num; ++i) {
		//TODO:失敗したら解放する。
		auto mem = node_mp->acquire(i);
		if (is_fail(mem))
			return null_pair(mem.cause());

		mempool::node* nd = new (mem.data()) mempool::node;
		if (!nd)
			return null_pair(cause::NOMEM);

		new_mp->set_node(i, nd);
	}

	new_mp->setup_mem_allocator(&_mp_allocator_ifs);

	exclusived_chain_lock.wlock();

	mempool* mp;
	for (mp = exclusived_chain.front(); mp; mp = exclusived_chain.next(mp))
	{
		if (objsize < mp->get_obj_size()) {
			exclusived_chain.insert_before(mp, new_mp);
			break;
		}
	}
	if (!mp)
		exclusived_chain.push_back(new_mp);

	exclusived_chain_lock.un_wlock();

	return make_pair(cause::OK, new_mp);
}

cause::t mempool_ctl::destroy_exclusived_mp(mempool* mp)
{
	cause::t r = mp->destroy();
	if (is_fail(r))
		return r;

	destroy_mp(mp);

	exclusived_chain_lock.wlock();

	exclusived_chain.remove(mp);

	exclusived_chain_lock.un_wlock();

	mp->~mempool();
	operator delete (mp, mp);

	mempool_mp->release(mp);

	return cause::OK;
}

namespace {

struct heap
{
	mempool* mp;
	u8 mem[];
};

heap* mem_to_heap(void* mem)
{
	uptr memadr = reinterpret_cast<uptr>(mem);

	return reinterpret_cast<heap*>(memadr - sizeof (mempool*));
}

}  // namespace

void* mempool_ctl::shared_allocate(u32 bytes)
{
	const u32 size = mempool::normalize_obj_size(bytes + sizeof (mempool*));

	mempool* pool = 0;
	for (mempool* mp : shared_chain) {
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

void mempool_ctl::shared_deallocate(void* mem)
{
	heap* h = mem_to_heap(mem);
	mempool* mp = h->mp;

	mp->dealloc(h);

	if (mp->get_shared_count() == 0 && mp->get_alloc_cnt() == 0) 
		mp->destroy();
}

void mempool_ctl::dump(output_buffer& ob)
{
	ob("name              obj_size   alloc_cnt    page_cnt freeobj_cnt")();
	for (mempool* mp : shared_chain)
		mp->dump_table(ob);
}

cause::t mempool_ctl::setup_mp()
{
	//const int cpuid = arch::get_cpu_node_id();
	// TODO:実行中の cpu_node_id を 0 と仮定している。
	const int cpuid = 0;

	// mempool を作成するための一時的な mempool。
	mempool tmp_mempool_mp(sizeof (mempool), arch::page::L1, 0);

	mempool::page* mempool_pg = tmp_mempool_mp.new_page(cpuid);
	if (!mempool_pg)
		return cause::NOMEM;

	// ここではまだ mem_allocator は使えないため、placement new を使う。

	mempool_mp = new (mempool_pg->acquire())
	    mempool(sizeof (mempool), arch::page::L1, 0);

	node_mp = new (mempool_pg->acquire())
	    mempool(sizeof (mempool::node), arch::page::L1, 0);

	if (!mempool_mp || !node_mp)
		return cause::NOMEM;

	const int cpu_num = get_cpu_node_count();
	for (int i = 0; i < cpu_num; ++i) {
		mempool::page* node_pg = node_mp->new_page(i);
		if (!node_pg)
			return cause::NOMEM;

		// node_mp->mempool_nodes を初期化する。
		mempool::node* node = new (node_pg->acquire()) mempool::node;
		if (!node)
			return cause::NOMEM;
		node_mp->set_node(i, node);

		node->import_dirty_page(node_pg);

		// mempool_mp->mempool_nodes を初期化する。
		node = new (node_pg->acquire()) mempool::node;
		if (!node)
			return cause::NOMEM;
		mempool_mp->set_node(i, node);

		if (i == cpuid)
			node->import_dirty_page(mempool_pg);
	}

	// offpage_mp を生成する。
	// offpage_mp は offpage mempool を生成するために必要。
	// offpage_mp 自身を offpage にすることはできない。
	auto mp = create_exclusived_mp(
	    sizeof (mempool::page), arch::page::INVALID, mempool::ONPAGE);
	if (is_fail(mp))
		return mp.cause();
	offpage_mp = mp.data();

	return cause::OK;
}

cause::t mempool_ctl::setup_shared_mp()
{
	for (uptr size = sizeof (cpu_word) * 4;
	          size <= sizeof (cpu_word) * 0x20000;
	          size *= 2)
	{
		mempool* mp;

		cause::t r = create_shared(size, &mp);
		if (is_fail(r))
			return r;

		mp->inc_shared_count();

		r = create_shared(size + size / 2, &mp);
		if (is_fail(r))
			return r;

		mp->inc_shared_count();
	}

	return cause::OK;
}

cause::t mempool_ctl::unsetup_mp()
{
	cause::t r = destroy_exclusived_mp(offpage_mp);
	if (is_fail(r))
		return r;

	const int cpu_num = get_cpu_node_count();
	for (int i = 0; i < cpu_num; ++i) {
	}

	mempool_mp->release(mempool_mp);
	mempool_mp->release(node_mp);

	mempool_mp->destroy();
	node_mp->destroy();

	destroy_mp(mempool_mp);
	destroy_mp(node_mp);

	mempool_mp->~mempool();
	node_mp->~mempool();

	operator delete (mempool_mp, mempool_mp);
	operator delete (node_mp, node_mp);

	return cause::OK;
}

cause::t mempool_ctl::unsetup_shared_mp()
{
	for (mempool* mp = shared_chain.front();
	              mp;
	              mp = shared_chain.next(mp))
	{
		destroy_shared(mp);
	}

	return cause::OK;
}

/// @brief  Find existing shared mem_cache.
mempool* mempool_ctl::find_shared(u32 objsize)
{
	for (mempool* mp : shared_chain) {
		if (mp->get_obj_size() >= objsize)
			return mp;
	}

	return 0;
}

/// @brief  shared mempool を新規に作る。
/// @param[in] objsize  オブジェクトサイズを指定する。
/// @param[out] new_mp  作成された shared mempool が返される。
//
/// @note  この関数を呼び出す前に、すでに同じサイズの shared mempool が
/// 無いことを保証する必要がある。
cause::t mempool_ctl::create_shared(u32 objsize, mempool** new_mp)
{
	arch::page::TYPE page_type = arch::page::INVALID;
	mempool::PAGE_STYLE page_style = mempool::ENTRUST;

	cause::t r = decide_params(&objsize, &page_type, &page_style);
	if (is_fail(r))
		return r;

	void* _mem = mempool_mp->alloc();
	mempool* _offpage_mp = page_style == mempool::ONPAGE ? 0 : offpage_mp;

	*new_mp = new (_mem) mempool(objsize, page_type, _offpage_mp);
	if (!*new_mp)
		return cause::NOMEM;

	const cpu_id cpu_num = get_cpu_node_count();
	for (cpu_id i = 0; i < cpu_num; ++i) {
		mempool::node* nd = new (node_mp->alloc(i)) mempool::node;
		if (!nd)
			return cause::NOMEM;

		(*new_mp)->set_node(i, nd);
	}

	for (mempool* mp = shared_chain.front();
	     mp;
	     mp = shared_chain.next(mp))
	{
		if (objsize < mp->get_obj_size()) {
			shared_chain.insert_before(mp, *new_mp);
			return cause::OK;
		}
	}

	shared_chain.push_back(*new_mp);

	return cause::OK;
}

void mempool_ctl::destroy_shared(mempool* mp)
{
	// TODO: return value
	mp->destroy();

	shared_chain.remove(mp);

	destroy_mp(mp);

	mp->~mempool();
	operator delete (mp, mp);

	mempool_mp->release(mp);
}

void mempool_ctl::destroy_mp(mempool* mp)
{
	const cpu_id cpu_num = get_cpu_node_count();
	for (cpu_id i = 0; i < cpu_num; ++i) {
		mempool::node* nd = mp->get_node(i);
		if (nd) {
			nd->~node();
			operator delete (nd, nd);

			node_mp->release(nd);
			mp->set_node(i, nullptr);
		}
	}
}

/// @brief  mempool を生成するときのパラメータを決定する。
/// @param[in,out] objsize  必要なオブジェクトサイズを入力すると、生成する
///                         mempool のオブジェクトサイズが返される。
/// @param[in,out] page_type 希望のページタイプを指定する。
///                          arch::page::INVALID を指定すれば適当なページタイプ
///                          が選択される。
/// @param[in,out] page_style ONPAGE / OFFPAGE / ENTRUST を指定する。
///                           ENTRUST を指定すると適当に選択する。
/// @retval cause::OK      成功した。
/// @retval cause::BADARG  パラメータを決定できなかった。
///                        objsize が大きすぎるとパラメータを決定できない。
cause::t mempool_ctl::decide_params(
    u32*                  objsize,
    arch::page::TYPE*     page_type,
    mempool::PAGE_STYLE*  page_style)
{
	*objsize = mempool::normalize_obj_size(*objsize);

	if (*page_type == arch::page::INVALID) {
		// お任せの場合は *objsize が8個収まるページを選択する。
		*page_type = arch::page::type_of_size(*objsize * 8);

		if (*page_type == arch::page::INVALID) {
			*page_type = arch::page::type_of_size(*objsize);
			if (*page_type == arch::page::INVALID)
				return cause::BADARG;
		}
	}

	if (*page_style == mempool::ENTRUST) {
		if (*objsize < (arch::page::L1_SIZE / 8)) {
			// 最低レベルページサイズ / 8 より小さければ ONPAGE。
			*page_style = mempool::ONPAGE;
		} else {
			// ページの中に *objsize を並べたときに余るサイズが
			// mempool::page 未満であれば OFFPAGE。
			const uptr page_size = size_of_type(*page_type);
			if ((page_size % *objsize) < sizeof (mempool::page))
				*page_style = mempool::OFFPAGE;
			else
				*page_style = mempool::ONPAGE;
		}
	}

	if (*page_style == mempool::ONPAGE) {
		const uptr page_size = size_of_type(*page_type);
		if ((*objsize + sizeof (mempool::page)) > page_size)
			return cause::BADARG;
	} else {
		if (*objsize > size_of_type(*page_type))
			return cause::BADARG;
	}

	return cause::OK;
}

mempool_ctl* get_mempool_ctl()
{
	return global_vars::core.mempool_ctl_obj;
}


// mempool_ctl::shared_mem_allocator

namespace {

cause::pair<void*> shared_mem_Allocate(mem_allocator*, uptr bytes)
{
	void* p = global_vars::core.mempool_ctl_obj->shared_allocate(bytes);
	if (p)
		return make_pair(cause::OK, p);
	else
		return null_pair(cause::NOMEM);
}

cause::t shared_mem_Deallocate(mem_allocator*, void* adr)
{
	global_vars::core.mempool_ctl_obj->shared_deallocate(adr);

	return cause::OK;
}

cause::pair<uptr> shared_mem_GetSize(mem_allocator*, void* mem)
{
	heap* h = mem_to_heap(mem);

	mempool* mp = h->mp;

	return cause::make_pair<uptr>(
	    cause::OK, mp->get_obj_size() - sizeof *h);
}

}  // namespace

void mempool_ctl::shared_mem_allocator::init()
{
	_ifs.init();
	_ifs.Allocate   = shared_mem_Allocate;
	_ifs.Deallocate = shared_mem_Deallocate;
	_ifs.GetSize    = shared_mem_GetSize;
}


/// @pre cpu_node instances are available.
cause::t mempool_setup()
{
	// 最初にスタック上に mempool_ctl を作成してから、
	// ヒープに mempool_ctl を作成し、中身を移動する。

	mempool_ctl tmp_mpctl;
	cause::t r = tmp_mpctl.setup();
	if (is_fail(r)) {
		tmp_mpctl.unsetup();
		return r;
	}

	void* buf = tmp_mpctl.shared_allocate(sizeof (mempool_ctl));
	mempool_ctl* mpctl = new (buf) mempool_ctl();
	if (!mpctl) {
		tmp_mpctl.unsetup();
		return cause::NOMEM;;
	}

	tmp_mpctl.move_to(mpctl);

	global_vars::core.mempool_ctl_obj = mpctl;

	return cause::OK;
}

/// @pre mem_io_setup() completed.
cause::t mempool_post_setup()
{
	return global_vars::core.mempool_ctl_obj->post_setup();
}

mem_allocator& generic_mem()
{
	return global_vars::core.mempool_ctl_obj->shared_mem();
}

