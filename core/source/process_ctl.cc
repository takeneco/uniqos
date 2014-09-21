/// @file   core/source/process_ctl.cc
/// @brief  process_ctl class implementation.

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
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


process_ctl::process_ctl()
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

