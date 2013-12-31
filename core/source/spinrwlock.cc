/// @file  spinrwlock.cc
/// @brief spin_rwlock definition.

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


// spin_rwlock

bool spin_rwlock::try_rlock()
{
	preempt_disable();

	if (spin_rwlock_ops::try_rlock())
		return true;

	preempt_enable();

	return false;
}

bool spin_rwlock::try_wlock()
{
	preempt_disable();

	if (spin_rwlock_ops::try_wlock())
		return true;

	preempt_enable();

	return false;
}

void spin_rwlock::rlock()
{
	for (;;) {
		preempt_disable();

		if (arch::spin_rwlock_ops::try_rlock())
			break;

		preempt_enable();

		arch::cpu_relax();
	}
}

void spin_rwlock::rlock_np()
{
	for (;;) {
		if (arch::spin_rwlock_ops::try_rlock())
			break;

		arch::cpu_relax();
	}
}

void spin_rwlock::wlock()
{
	for (;;) {
		preempt_disable();

		if (arch::spin_rwlock_ops::try_wlock())
			break;

		preempt_enable();

		arch::cpu_relax();
	}
}

void spin_rwlock::wlock_np()
{
	for (;;) {
		if (arch::spin_rwlock_ops::try_wlock())
			break;

		arch::cpu_relax();
	}
}

void spin_rwlock::un_rlock()
{
	spin_rwlock_ops::un_rlock();

	preempt_enable();
}

void spin_rwlock::un_rlock_np()
{
	spin_rwlock_ops::un_rlock();
}

void spin_rwlock::un_wlock()
{
	spin_rwlock_ops::un_wlock();

	preempt_enable();
}

void spin_rwlock::un_wlock_np()
{
	spin_rwlock_ops::un_wlock();
}

