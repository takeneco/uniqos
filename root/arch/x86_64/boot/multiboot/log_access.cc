/// @file   log_access.cc
/// @brief  global log interface.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
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

#include "misc.hh"

#include <log_target.hh>


namespace {

enum { LOG_MODES = 2 };
log_target log_tgt[LOG_MODES];

}  // namespace

void log_set(uint i, io_node* target)
{
	log_target::setup();

	if (i < LOG_MODES)
		log_tgt[i].install(target, 0);
}

log::log(int i)
:    output_buffer(&log_tgt[i], 0)
{
}

log::~log()
{
	flush();
}

