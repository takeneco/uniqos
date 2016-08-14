/// @file   native_process_ctl.cc
/// @brief  process_ctl implementation for x86_64

//  Uniqos  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include <core/process_ctl.hh>

#include "native_process.hh"
#include <core/global_vars.hh>
#include <core/new_ops.hh>


namespace x86 {

class native_process_ctl : public process_ctl
{
public:
	native_process_ctl();

	cause::t setup();

private:
	cause::t setup_self_process();
};


native_process_ctl::native_process_ctl()
{
}

cause::t native_process_ctl::setup()
{
	cause::t r = process_ctl::setup();
	if (is_fail(r))
		return r;

	r = setup_self_process();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::t native_process_ctl::setup_self_process()
{
	native_process* self_prc = new (generic_mem()) native_process;

	cause::t r = self_prc->setup_self();
	if (is_fail(r))
		return r;

	r = process_ctl::add(self_prc);
	if (is_fail(r))
		return r;

	return cause::OK;
}

/// @brief Initialize native_process_ctl.
/// @pre mempool_init() was successful.
/// @pre get_current_native_thread() is available.
cause::t native_process_init()
{
	native_process_ctl* obj = new (generic_mem()) native_process_ctl;
	if (!obj)
		return cause::NOMEM;

	cause::t r = obj->setup();
	if (is_fail(r)) {
		new_destroy(obj, generic_mem());
		return r;
	}

	global_vars::core.process_ctl_obj = obj;

	return cause::OK;
}

}  // namespace x86

