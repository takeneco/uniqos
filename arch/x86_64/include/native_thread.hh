/// @file  native_thread.hh
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_NATIVE_THREAD_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_THREAD_HH_

#include <regset.hh>
#include <thread.hh>


class cpu_node;

namespace x86 {

class native_thread : public thread
{
	DISALLOW_COPY_AND_ASSIGN(native_thread);

public:
	native_thread(uptr text, uptr param, uptr stack_size);

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

typedef void (*entry_point)(void* context);
inline cause::pair<native_thread*> create_thread(
    cpu_node* owner_cpu,
    entry_point text,
    void* param)
{
	return create_thread(owner_cpu,
	    reinterpret_cast<uptr>(text),
	    reinterpret_cast<uptr>(param));
}

}  // namespace x86


#endif  // include guard

