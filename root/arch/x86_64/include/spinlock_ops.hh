/// @file spinlock_ops.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_SPINLOCK_OPS_HH_
#define ARCH_X86_64_INCLUDE_SPINLOCK_OPS_HH_

#include <basic.hh>


namespace arch {

class spin_rwlock_ops
{
protected:

#if CONFIG_MAX_CPU < 0x100
	enum { WLOCK = -0x100 };
	typedef s16 atom_type;

#elif CONFIG_MAX_CPU < 0x10000
	enum { WLOCK = -0x10000 };
	typedef s32 atom_type;

#elif CONFIG_MAX_CPU < 0x100000000
	enum { WLOCK = U64(-0x100000000) };
	typedef s64 atom_type;

#else  // CONFIG_MAX_CPU
# error "Too big CONFIG_MAX_CPU."

#endif  // CONFIG_MAX_CPU

	volatile atom_type atom;

	spin_rwlock_ops() : atom(0) {}

	bool can_rlock() { return atom >= 0; }
	bool can_wlock() { return atom == 0; }
	bool try_rlock();
	bool try_wlock();
	void un_rlock();
	void un_wlock();
};

inline void cpu_relax() {
	asm ("pause");
}

};


#endif  // include guard

