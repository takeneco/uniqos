/// @file  spinlock.cc
/// @brief Spinlock.

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

#include <spinlock.hh>

#include <cpu_node.hh>

#include <native_ops.hh>


namespace {

inline void local_preempt_disable()
{
#ifdef KERNEL
	preempt_disable();
#endif
}

inline void local_preempt_enable()
{
#ifdef KERNEL
	preempt_enable();
#endif
}

}  // namespace

// spin_lock

void spin_lock::lock()
{
	for (;;) {
		local_preempt_disable();

		if (_try_lock())
			break;

		local_preempt_enable();

		arch::cpu_relax();
	}
}

void spin_lock::lock_np()
{
	for (;;) {
		if (_try_lock())
			break;

		arch::cpu_relax();
	}
}

bool spin_lock::try_lock()
{
	local_preempt_disable();

	const bool r = _try_lock();

	if (!r)
		local_preempt_enable();

	return r;
}

void spin_lock::unlock()
{
	atom.store(UNLOCKED);

	local_preempt_enable();
}

void spin_lock::unlock_np()
{
	atom.store(UNLOCKED);
}

