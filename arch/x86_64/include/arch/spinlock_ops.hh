/// @file spinlock_ops.hh

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

