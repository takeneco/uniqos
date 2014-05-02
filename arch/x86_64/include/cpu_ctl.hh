/// @file  cpu_ctl.hh
/// @brief CPU shared resource.
//
// (C) 2011-2014 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_CPU_CTL_HH_
#define ARCH_X86_64_INCLUDE_CPU_CTL_HH_

#include <cpu_idte.hh>
#include <mpspec.hh>


// call by native_cpu_ctl::IDT
cause::t intr_init(idte* idt);

namespace x86 {

/// Architecture dependent part of processor control.
class native_cpu_ctl
{
public:
	native_cpu_ctl();
	cause::t setup();
	cause::t load();

	const mpspec* get_mpspec() const {
		return &mps;
	}

private:
	class IDT
	{
	public:
		cause::t init() {
			return intr_init(idt);
		}

		idte* get() { return idt; }

	private:
		idte idt[256];
	};

private:
	mpspec mps;
	IDT idt;
};

}  // namespace x86


#endif  // include guard

