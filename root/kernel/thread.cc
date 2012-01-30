// @file   thread.cc
// @brief  thread class implements.
//
// (C) 2012 KATO Takeshi
//

#include <thread.hh>


thread::thread(uptr text, uptr param, uptr stack, uptr stack_size) :
	rs(text, param, stack, stack_size)
{
}

