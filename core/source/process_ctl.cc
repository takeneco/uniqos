/// @file   process_ctl.cc
/// @brief  process_ctl class implementation.

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include <core/process_ctl.hh>

#include <core/global_vars.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>
#include <core/setup.hh>


process_ctl::process_ctl() :
	io_desc_mp(nullptr)
{
}

process_ctl::~process_ctl()
{
}

cause::t process_ctl::setup()
{
	auto mp = mempool::create_exclusive(
	    sizeof (io_desc), arch::page::INVALID, mempool::ENTRUST);
	if (is_fail(mp))
		return mp.get_cause();

	io_desc_mp = mp;

	return process_id_map.init(10);
}

cause::t process_ctl::unsetup()
{
	if (io_desc_mp) {
		auto r = mempool::destroy_exclusive(io_desc_mp);
		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

cause::t process_ctl::add(process* prc)
{
	cause::t r = process_id_map.insert(prc);
	if (is_fail(r))
		return r;

	return cause::OK;
}

process_ctl* get_process_ctl()
{
	return global_vars::core.process_ctl_obj;
}

