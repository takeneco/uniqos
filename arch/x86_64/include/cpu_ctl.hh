/// @file  cpu_ctl.hh
//
// (C) 2011-2013 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_CPU_CTL_HH_
#define ARCH_X86_64_KERNEL_CPU_CTL_HH_

#include <cpu_idte.hh>
#include <mpspec.hh>
#include <regset.hh>


class thread;
class cpu_node;

// call by cpu_ctl::IDT
cause::t intr_init(idte* idt);

namespace arch {

/// Architecture dependent part of processor control.
class cpu_ctl
{
public:
	class IDT;

protected:
	cause::t setup();
};


// 別ヘッダへ移動予定
class cpu_ctl::IDT
{
public:
	cause::t init() {
		return intr_init(idt);
	}

	idte* get() { return idt; }

private:
	idte idt[256];
};

}  // namespace arch

/// CPUの共有データ
class cpu_ctl_common
{
public:
	cpu_ctl_common();

	cause::t init();

	cause::t setup_idt();

	const mpspec* get_mpspec() const {
		return &mps;
	}

private:
	mpspec mps;

	arch::cpu_ctl::IDT idt;
};


#endif  // include guard

