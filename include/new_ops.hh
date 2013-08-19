/// @file   new_ops.hh
//
// (C) 2010,2013 KATO Takeshi
//

#ifndef INCLUDE_NEW_OPS_HH_
#define INCLUDE_NEW_OPS_HH_

#include <basic.hh>


inline void* operator new  (uptr, void* ptr) { return ptr; }
inline void* operator new[](uptr, void* ptr) { return ptr; }

inline void operator delete  (void*, void*) {}
inline void operator delete[](void*, void*) {}


#endif  // include guard

