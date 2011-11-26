/// @file   cheap_alloc_use.cc
//
// (C) 2010-2011 KATO Takeshi
//

#include "misc.hh"
#include "placement_new.hh"


namespace {

// コンストラクタを呼ばせない。
uptr mem_buf_[sizeof (allocator) / sizeof (uptr)];

}  // namespace

inline allocator* get_alloc() {
	return reinterpret_cast<allocator*>(mem_buf_);
}


/// @brief  Initialize alloc.
void init_alloc()
{
	new (get_alloc()) allocator;
}

