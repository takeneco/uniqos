/// @file   syscall_entry.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
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

#include <basic.hh>
#include <log.hh>



namespace {

int i;

}  // namespace

extern "C" cause::pair<uptr> syscall_entry(const void* data)
{
	if (i++ % 0xffff == 0)
		log()('#');
	return cause::pair<uptr>(cause::OK, 0);
}

