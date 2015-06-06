/// @file   vadr_pool.cc
/// @brief  Virtual address pool.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
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

#include <core/vadr_pool.hh>

#include <arch.hh>
#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/new_ops.hh>


// vadr_pool::resource

uptr vadr_pool::resource::get_vadr(uptr padr)
{
	return vadr_range.low_adr() + (padr - padr_range.low_adr());
}

// vadr_pool

vadr_pool::vadr_pool(uptr _pool_start, uptr _pool_end) :
	pool_start(_pool_start),
	pool_end(_pool_end),
	resource_mp(nullptr)
{
}

cause::t vadr_pool::setup()
{
	auto r = mempool::acquire_shared(sizeof (resource));
	if (r.is_fail())
		return r.cause();

	resource_mp = r.data();

	resource* res = new (*resource_mp) resource;
	if (!res)
		return cause::NOMEM;

	res->vadr_range.set_lh(pool_start, pool_end);

	pool_chain.push_front(res);

	return cause::OK;
}

cause::t vadr_pool::unsetup()
{
	resource* res;
	for (;;) {
		res = pool_chain.front();
		if (!res)
			break;
		pool_chain.remove(res);
		new_destroy(res, *resource_mp);
	}
	for (;;) {
		res = assign_chain.front();
		if (!res)
			break;
		assign_chain.remove(res);
		new_destroy(res, *resource_mp);
	}

	if (resource_mp)
		mempool::release_shared(resource_mp);

	return cause::OK;
}

/// @brief 仮想アドレスを物理アドレスにマップし、仮想アドレスを返す。
cause::pair<void*> vadr_pool::assign(
    uptr padr, uptr bytes, page_flags flags)
{
	spin_lock_section _sls(lock);

	cause::pair<resource*> _res = assign_by_assign(padr, bytes, flags);
	if (_res.cause() == cause::FAIL) {
		_res = assign_by_pool(padr, bytes, flags);
		if (_res.is_fail())
			return null_pair(_res.cause());
	}

	resource* res = _res.data();

	return make_pair(cause::OK,
	                 reinterpret_cast<void*>(res->get_vadr(padr)));
}

/// @brief 仮想アドレスを返却する。
cause::t vadr_pool::revoke(void* vadr)
{
	spin_lock_section _sls(lock);

	uptr _vadr = reinterpret_cast<uptr>(vadr);

	resource* res = nullptr;
	for (resource* _res : assign_chain) {
		if(res->vadr_range.test(_vadr)) {
			res = _res;
			break;
		}
	}

	if (!res)
		return cause::NOENT;

	--res->ref_cnt;

	if (res->ref_cnt == 0) {
		assign_chain.remove(res);
		cause::t r = page_unmap(nullptr,
		                        res->vadr_range.low_adr(),
		                        res->pagelevel);
		if (is_fail(r))
			log()(SRCPOS)("!!! page_unmap() failed.")();

		r = merge_pool(res);
		if (is_fail(r))
			log()(SRCPOS)("!!! vadr_pool::merge_pool() failed")();
	}

	return cause::OK;
}

/// @brief  assign_chainから条件に合うvadrを探す。
/// @retval cause::OK   Succeeded.
/// @retval cause::FAIL Not found.
auto vadr_pool::assign_by_assign(
    uptr padr, uptr bytes, page_flags flags)
-> cause::pair<resource*>
{
	uptr padr_low = padr;
	uptr padr_high = padr + bytes - 1;
	for (resource* res : assign_chain) {
		if (res->padr_range.test(padr_low) &&
		    res->padr_range.test(padr_high) &&
		    res->pageflags == flags)
		{
			++res->ref_cnt;
			return make_pair(cause::OK, res);
		}
	}

	return null_pair(cause::FAIL);
}

auto vadr_pool::assign_by_pool(
    uptr padr, uptr bytes, page_flags flags)
-> cause::pair<resource*>
{
	page_level level = arch::page::PHYS_L2;

	int page_bits = arch::page::bits_of_level(level);
	uptr page_size = UPTR(1) << page_bits;
	uptr page_mask = ~(page_size - 1);
	uptr page_low = padr & page_mask;
	uptr page_high = page_low + page_size -1;

	if ((padr + bytes) > page_high)
		return null_pair(cause::BADARG);

	auto _res = cut_resource(page_size);
	if (is_fail(_res))
		return _res;

	resource* res = _res.data();
	res->padr_range.set_ab(page_low, page_size);
	res->pagelevel = level;
	res->pageflags = flags;
	res->ref_cnt = 1;
	cause::t r = page_map(
	    nullptr,
	    res->vadr_range.low_adr(),
	    res->padr_range.low_adr(),
	    res->pagelevel,
	    res->pageflags);
	if (is_fail(r)) {
		cause::t r2 = merge_pool(res);
		if (is_fail(r2))
			log()(SRCPOS)("vadr_pool::merge_pool() failed.")();
		return null_pair(r);
	}

	assign_chain.push_front(res);

	return make_pair(cause::OK, res);
}

/// @brief  pool_chainからpage_sizeだけ切り出す。
/// @retval cause::OK     Succeeded.
/// @retval cause::NODEV  pool_chainに空きアドレスが無い。
/// @retval cause::MEM    管理用メモリが無い。
auto vadr_pool::cut_resource(uptr page_size)
-> cause::pair<resource*>
{
	resource* res = nullptr;
	uptr low;  // assign low vadr
	uptr high; // assign high vadr
	for (resource* _res : pool_chain) {
		low = _res->vadr_range.low_adr();
		low = up_align<uptr>(low, page_size);
		high = _res->vadr_range.high_adr();
		if (low < high && page_size <= (high - low + 1)) {
			res = _res;
			high = low + page_size - 1;
			break;
		}
	}
	if (!res)
		return null_pair(cause::NODEV);

	resource* pool_low = nullptr;
	resource* pool_high = nullptr;

	if (low != res->vadr_range.low_adr()) {
		pool_low = new (*resource_mp) resource;
		if (!pool_low)
			return null_pair(cause::NOMEM);

		pool_low->vadr_range.set_lh(res->vadr_range.low_adr(),
		                            low - 1);
	}

	if (high != res->vadr_range.high_adr()) {
		pool_high = new (*resource_mp) resource;
		if (!pool_high) {
			new_destroy(pool_low, *resource_mp);
			return null_pair(cause::NOMEM);
		}

		pool_high->vadr_range.set_lh(high + 1,
		                             res->vadr_range.high_adr());
	}

	res->vadr_range.set_lh(low, high);

	if (pool_low)
		pool_chain.push_front(pool_low);

	if (pool_high)
		pool_chain.push_front(pool_high);

	pool_chain.remove(res);

	return make_pair(cause::OK, res);
}

cause::t vadr_pool::merge_pool(resource* res)
{
	for (resource* _res : pool_chain) {
		if ((_res->vadr_range.high_adr() + 1) ==
		     res->vadr_range.low_adr())
		{
			pool_chain.remove(_res);
			res->vadr_range.set_lh(
			    _res->vadr_range.low_adr(),
			    res->vadr_range.high_adr());
			new_destroy(_res, *resource_mp);
			break;
		}
	}

	for (resource* _res : pool_chain) {
		if ((res->vadr_range.high_adr() + 1) ==
		     _res->vadr_range.low_adr())
		{
			pool_chain.remove(_res);
			res->vadr_range.set_lh(
			    res->vadr_range.low_adr(),
			    _res->vadr_range.high_adr());
			new_destroy(_res, *resource_mp);
			break;
		}
	}

	pool_chain.push_front(res);

	return cause::OK;
}


cause::t vadr_pool_setup()
{
	vadr_pool* vap = new (generic_mem())
	    vadr_pool(arch::VADRPOOL_START, arch::VADRPOOL_END);
	if (!vap)
		return cause::NOMEM;

	cause::t r = vap->setup();
	if (is_fail(r)) {
		cause::t r2 = vap->unsetup();
		if (is_fail(r2))
			log()("vadr_pool::unsetup() failed. r=").u(r2)();
		return r;
	}

	global_vars::core.vadr_pool_obj = vap;

	return cause::OK;
}

