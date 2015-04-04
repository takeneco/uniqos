/// @file   cheap_alloc_use.cc

//  UNIQOS  --  Unique Operating System
//  (C) 2011,2015 KATO Takeshi
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


inline void* operator new  (uptr, void* ptr) { return ptr; }

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

