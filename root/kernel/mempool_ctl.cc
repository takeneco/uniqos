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

#include <cpu_node.hh>
#include <global_vars.hh>
#include <log.hh>
#include <placement_new.hh>


void* mem_alloc(u32 bytes)
{
	return global_vars::core.mempool_ctl_obj->shared_alloc(bytes);
}

void mem_dealloc(void* mem)
{
	return global_vars::core.mempool_ctl_obj->shared_dealloc(mem);
}


// mempool_ctl

mempool_ctl::mempool_ctl(
    mempool* _mempool_mp, mempool* _node_mp, mempool* _own_mp) :
	offpage_mp(0),
	mempool_mp(_mempool_mp),
	node_mp(_node_mp),
	own_mp(_own_mp)
{
}

cause::type mempool_ctl::init()
{
	// offpage_mp を生成する。
	// offpage_mp は offpage mempool を生成するために必要。
	// offpage_mp 自身を offpage にすることはできない。
	cause::type r = exclusived_mempool(sizeof (mempool::page),
	                                   arch::page::INVALID,
	                                   ONPAGE,
	                                   &offpage_mp);
	if (is_fail(r))
		return r;

	r = init_shared();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::type mempool_ctl::shared_mempool(u32 objsize, mempool** mp)
{
	objsize = mempool::normalize_obj_size(objsize);

	mempool* _mp = find_shared(objsize);

	if (!mp) {
		cause::type r = create_shared(objsize, &_mp);
		if (is_fail(r))
			return r;
	}

	_mp->inc_shared_count();

	*mp = _mp;

	return cause::OK;
}

void mempool_ctl::release_shared_mempool(mempool* mp)
{
	if (mp->dec_shared_count() == 0 && mp->get_alloc_count() == 0) 
		mp->destroy();
}

/// @brief  用途限定の mempool を生成する。
cause::type mempool_ctl::exclusived_mempool(
    u32 objsize,                ///< [in] オブジェクトサイズ。
    arch::page::TYPE page_type, ///< [in] ページタイプを指定する。
                                ///<      INVALID を指定すると適当なものが
				///<      選択される。
    PAGE_STYLE page_style,      ///< [in] ONPAGE/OFFPAGE/ENTRUST を指定する。
    mempool** new_mp)           ///< [out] 生成された mempool が返される。
{
	cause::type r = decide_params(&objsize, &page_type, &page_style);
	if (is_fail(r))
		return cause::FAIL;

	mempool* opp = page_style == ONPAGE ? 0 : offpage_mp;

	*new_mp = new (mempool_mp->alloc()) mempool(objsize, page_type, opp);

	if (!*new_mp)
		return cause::NOMEM;

	const cpu_id cpu_num = get_cpu_node_count();
	for (int i = 0; i < cpu_num; ++i) {
		mempool::node* nd = new (node_mp->alloc(i)) mempool::node;
		if (!nd)
			return cause::NOMEM;

		(*new_mp)->set_node(i, nd);
	}

	return cause::OK;
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

void mempool_ctl::shared_dealloc(void* mem)
{
	uptr memadr = reinterpret_cast<uptr>(mem);

	heap* h = reinterpret_cast<heap*>(memadr - sizeof (mempool*));
	mempool* mp = h->mp;

	mp->dealloc(mem);

	if (mp->get_shared_count() == 0 && mp->get_alloc_count() == 0) 
		mp->destroy();
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

cause::type mempool_ctl::init_shared()
{
	const uptr sizes[] = {
		0x1000, 0x2000, 0x3000, 0x4000, 0x200000,
	};

	for (uptr i = 0; i < num_of_array(sizes); ++i) {
		mempool* mp;
		const cause::type r = create_shared(sizes[i], &mp);
		if (is_fail(r))
			return r;

		mp->inc_shared_count();
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

/// @brief  shared mempool を新規に作る。
/// @param[in] objsize  オブジェクトサイズを指定する。
/// @param[out] new_mp  作成された shared mempool が返される。
//
/// @note  この関数を呼び出す前に、すでに同じサイズの shared mempool が
/// 無いことを保証する必要がある。
cause::type mempool_ctl::create_shared(u32 objsize, mempool** new_mp)
{
	arch::page::TYPE page_type = arch::page::INVALID;
	PAGE_STYLE page_style = ENTRUST;

	cause::type r = decide_params(&objsize, &page_type, &page_style);
	if (is_fail(r))
		return r;

	void* _mem = mempool_mp->alloc();
	mempool* _offpage_mp = page_style == ONPAGE ? 0 : offpage_mp;

	*new_mp = new (_mem) mempool(objsize, page_type, _offpage_mp);
	if (!*new_mp)
		return cause::NOMEM;

	const cpu_id cpu_num = get_cpu_node_count();
	for (int i = 0; i < cpu_num; ++i) {
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
			shared_chain.insert_prev(mp, *new_mp);

			return cause::OK;
		}
	}

	shared_chain.insert_tail(*new_mp);

	return cause::OK;
}

/// @brief  mempool を生成するときのパラメータを決定する。
/// @param[in,out] objsize  必要なオブジェクトサイズを入力すると、生成する
///                         mempool のオブジェクトサイズが返される。
/// @param[in,out] page_type 希望のページタイプを指定する。
///                          arch::page::INVALID を指定すれば適当なページタイプ
///                          が選択される。
/// @param[in,out] page_style ONPAGE / OFFPAGE / ENTRUST を指定する。
///                           ENTRUST を指定すると適当に選択する。
cause::type mempool_ctl::decide_params(
    u32*              objsize,
    arch::page::TYPE* page_type,
    PAGE_STYLE*       page_style)
{
	*objsize = mempool::normalize_obj_size(*objsize);

	if (*page_type == arch::page::INVALID) {
		// お任せの場合は *objsize が8個収まるページを選択する。
		*page_type = arch::page::type_of_size(*objsize * 8);

		if (*page_type == arch::page::INVALID) {
			*page_type = arch::page::type_of_size(*objsize);
			if (*page_type == arch::page::INVALID)
				return cause::FAIL;
		}
	}

	if (*page_style == ENTRUST) {
		if (*objsize < (arch::page::L1_SIZE / 8)) {
			// 最低レベルページサイズ / 8 より小さければ ONPAGE。
			*page_style = ONPAGE;
		} else {
			// ページの中に *objsize を並べたときに余るサイズが
			// mempool::page 未満であれば OFFPAGE。
			const uptr page_size = size_of_type(*page_type);
			if ((page_size % *objsize) < sizeof (mempool::page))
				*page_style = OFFPAGE;
			else
				*page_style = ONPAGE;
		}
	}

	if (*page_style == ONPAGE) {
		const uptr page_size = size_of_type(*page_type);
		if ((*objsize + sizeof (mempool::page)) > page_size)
			return cause::FAIL;
	} else {
		if (*objsize > size_of_type(*page_type))
			return cause::FAIL;
	}

	return cause::OK;
}

/// @brief mempool_ctl を生成する。
//
/// mempool を使って mempool_ctl を生成するために、mempool も生成する。
/// mempool に含まれる mempool:node も生成する。
cause::stype mempool_ctl::create_mempool_ctl(mempool_ctl** mpctl)
{
	const int cpuid = arch::get_cpu_id();

	// mempool を作成するための一時的な mempool。
	mempool tmp_mempool_mp(sizeof (mempool), arch::page::L1, 0);
	mempool::page* mempool_pg = tmp_mempool_mp.new_page(cpuid);
	if (!mempool_pg)
		return cause::NOMEM;

	mempool* node_mp = new (mempool_pg->alloc())
	    mempool(sizeof (mempool::node), arch::page::L1, 0);
	if (!node_mp)
		return cause::NOMEM;

	mempool* mempool_mp = new (mempool_pg->alloc())
	    mempool(sizeof (mempool), arch::page::L1, 0);
	if (!mempool_mp)
		return cause::NOMEM;

	mempool* ctl_mp = new (mempool_pg->alloc())
	    mempool(sizeof (mempool_ctl), arch::page::L1, 0);
	if (!ctl_mp)
		return cause::NOMEM;

	const int cpu_num = get_cpu_node_count();
	for (int i = 0; i < cpu_num; ++i) {
		mempool::page* node_pg = node_mp->new_page(i);
		if (!node_pg)
			return cause::NOMEM;

		// node_mp->mempool_nodes を初期化する。
		mempool::node* node = new (node_pg->alloc()) mempool::node;
		if (!node)
			return cause::NOMEM;
		node_mp->set_node(i, node);

		node->include_dirty_page(node_pg);

		// mempool_mp->mempool_nodes を初期化する。
		node = new (node_pg->alloc()) mempool::node;
		if (!node)
			return cause::NOMEM;
		mempool_mp->set_node(i, node);

		if (i == cpuid)
			node->include_dirty_page(mempool_pg);

		// ctl_mp->mempool_nodes を初期化する。
		node = new (node_pg->alloc()) mempool::node;
		if (!node)
			return cause::NOMEM;
		ctl_mp->set_node(i, node);
	}

	mempool_ctl* mpc =
	    new (ctl_mp->alloc()) mempool_ctl(mempool_mp, node_mp, ctl_mp);
	if (!mpc)
		return cause::NOMEM;

	*mpctl = mpc;

	return cause::OK;
}

extern "C" cause::type mempool_create_shared(u32 objsize, mempool** mp)
{
	return global_vars::core.mempool_ctl_obj->shared_mempool(objsize, mp);
}

extern "C" void mempool_release_shared(mempool* mp)
{
	global_vars::core.mempool_ctl_obj->release_shared_mempool(mp);
}

cause::type mempool_init()
{
	mempool_ctl* mpctl;
	cause::type r = mempool_ctl::create_mempool_ctl(&mpctl);
	if (is_fail(r))
		return r;

	global_vars::core.mempool_ctl_obj = mpctl;

	r = mpctl->init();
	if (is_fail(r))
		return r;

	return cause::OK;
}

