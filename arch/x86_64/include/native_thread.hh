/// @file  native_thread.hh
//
// (C) 2013-2015 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_NATIVE_THREAD_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_THREAD_HH_

#include <regset.hh>
#include <core/thread.hh>


class cpu_node;
class process;

namespace x86 {

class native_thread : public thread
{
	DISALLOW_COPY_AND_ASSIGN(native_thread);

public:
	native_thread(thread_id tid);
	native_thread(thread_id tid, uptr text, uptr param, uptr stack_size);

	arch::regset* ref_regset() { return &rs; }

	uptr get_thread_private_info() const { return thread_private_info; }
	void set_thread_private_info(uptr info) { thread_private_info = info; }

public:
	arch::regset rs;
	uptr stack_bytes;
	/// swapgs でアクセスできる値
	uptr thread_private_info;
};

cause::pair<native_thread*> create_thread(
    cpu_node* owner_cpu, uptr text, uptr param);

typedef void (*thread_entry_point)(void* context);
cause::pair<native_thread*> create_thread(
    cpu_node* owner_cpu, thread_entry_point text, void* param);

cause::t destroy_thread(native_thread* t);

native_thread* get_current_native_thread();

}  // namespace x86


#endif  // include guard

