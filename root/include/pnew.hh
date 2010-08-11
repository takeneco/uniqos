/// @file   include/pnew.hh
/// @author Kato Takeshi
/// @brief  Placement new operator.
/// 明示的にコンストラクタ／デストラクタを呼び出すために使う。
///
/// (C) 2010 Kato Takeshi

#ifndef _INCLUDE_PNEW_HH_
#define _INCLUDE_PNEW_HH_


inline void* operator new  (unsigned long, void* ptr) { return ptr; }
inline void* operator new[](unsigned long, void* ptr) { return ptr; }

inline void operator delete  (void*, void*) {}
inline void operator delete[](void*, void*) {}


#endif  // Include guard.
