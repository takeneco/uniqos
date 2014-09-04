/// @file  native_process.hh
//
// (C) 2014 KATO Takeshi
//

#ifndef ARCH_X86_64_SOURCE_NATIVE_PROCESS_HH_
#define ARCH_X86_64_SOURCE_NATIVE_PROCESS_HH_

#include "page_table.hh"
#include <core/process.hh>


class thread;

namespace x86 {

class native_process : public process
{
public:
	native_process();

	cause::t init(thread* entry_thread, int iod_nr);

	page_table& ref_ptbl() { return ptbl; }

private:
	page_table ptbl;
};

}  // namespace x86


#endif  // include guard

