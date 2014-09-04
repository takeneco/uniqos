/// @file   core/include/core/process_ctl.hh
/// @brief  process_ctl class declaration.

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

#ifndef CORE_PROCESS_CTL_HH_
#define CORE_PROCESS_CTL_HH_

#include <core/numeric_map.hh>
#include <core/process.hh>


class process_ctl
{
	typedef numeric_map<
	    process_id,
	    process,
	    &process::get_process_id,
	    &process::get_process_ctl_node
	> process_id_map_type;

public:
	process_ctl();
	~process_ctl();

	cause::t init();

private:
	process_id_map_type process_id_map;
};


#endif  // include guard

