// @file   thread.cc
// @brief  thread class implements.
//
// (C) 2012 KATO Takeshi
//

#include <processor.hh>


thread::thread(
    processor* _owner,
    uptr text,
    uptr param,
    uptr stack,
    uptr stack_size
) :
	owner(_owner),
	rs(text, param, stack, stack_size)
{
	sleep_cancel_cmd = false;
}

void thread::ready()
{
	owner->get_thread_ctl().ready_thread(this);
}

