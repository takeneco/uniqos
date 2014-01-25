/// @file   native_process_ctl.cc
/// @brief  process_ctl implementation for x86_64

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

#include <global_vars.hh>
#include <new_ops.hh>


namespace x86 {

class native_process_ctl : public process_ctl
{
public:
	native_process_ctl();

	cause::t init();
};


native_process_ctl::native_process_ctl()
{
}

cause::t native_process_ctl::init()
{
	return process_ctl::init();
}

/// @brief Initialize native_process_ctl.
/// @pre mempool_init() was successful.
cause::t native_process_init()
{
	native_process_ctl* obj =
	    new (mem_alloc(sizeof (native_process_ctl))) native_process_ctl;
	if (!obj)
		return cause::NOMEM;

	cause::t r = obj->init();
	if (is_fail(r)) {
		obj->~native_process_ctl();
		mem_dealloc(obj);
		return r;
	}

	global_vars::arch.native_process_ctl_obj = obj;

	return cause::OK;
}

}  // namespace x86

