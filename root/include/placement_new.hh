/// @file   placement_new.hh
//
// (C) 2010 KATO Takeshi
//

#ifndef INCLUDE_PLACEMENT_NEW_HH_
#define INCLUDE_PLACEMENT_NEW_HH_

#include "basic_types.hh"


inline void* operator new  (uptr, void* ptr) { return ptr; }
inline void* operator new[](uptr, void* ptr) { return ptr; }

inline void operator delete  (void*, void*) {}
inline void operator delete[](void*, void*) {}


#endif  // include guard

