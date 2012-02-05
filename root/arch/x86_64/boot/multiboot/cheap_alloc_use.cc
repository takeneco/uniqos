/// @file   cheap_alloc_use.cc

//  uniqos  --  Unique Operating System
//  (C) 2011 KATO Takeshi
//
//  uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "misc.hh"
#include <placement_new.hh>


namespace {

// コンストラクタを呼ばせない。
uptr mem_buf_[sizeof (allocator) / sizeof (uptr)];

}  // namespace

inline allocator* get_alloc() {
	return reinterpret_cast<allocator*>(mem_buf_);
}


/// @brief  Initialize allocator.
void init_alloc()
{
	new (get_alloc()) allocator;
}

