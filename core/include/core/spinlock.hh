/// @file core/spinlock.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CORE_SPINLOCK_HH_
#define CORE_SPINLOCK_HH_

#include <util/atomic.hh>
#include <arch/spinlock_ops.hh>


class spin_lock
{
	DISALLOW_COPY_AND_ASSIGN(spin_lock);

	enum {
		UNLOCKED = 0,
		LOCKED = 1,
	};
	atomic<u8> atom;

public:
	spin_lock() : atom(UNLOCKED) {}

	bool is_locked() const {
		return atom.load() == LOCKED;
	}
	bool is_unlocked() const {
		return atom.load() == UNLOCKED;
	}

	void lock();
	void lock_np();
	bool try_lock();
	bool try_lock_np();

	void unlock();
	void unlock_np();

private:
	bool _try_lock() {
		return atom.exchange(LOCKED) == UNLOCKED;
	}
};


class spin_lock_section
{
	DISALLOW_COPY_AND_ASSIGN(spin_lock_section);

public:
	spin_lock_section(spin_lock& lock) :
		_lock(lock)
	{
		_lock.lock();
	}
	~spin_lock_section()
	{
		_lock.unlock();
	}

private:
	spin_lock& _lock;
};

class spin_lock_section_np
{
	DISALLOW_COPY_AND_ASSIGN(spin_lock_section_np);

public:
	spin_lock_section_np(spin_lock& lock) :
		_lock(lock)
	{
		_lock.lock_np();
	}
	~spin_lock_section_np()
	{
		_lock.unlock_np();
	}

private:
	spin_lock& _lock;
};

class spin_rwlock : public arch::spin_rwlock_ops
{
	DISALLOW_COPY_AND_ASSIGN(spin_rwlock);

public:
	spin_rwlock() {}

	bool try_rlock();
	bool try_wlock();

	void rlock();
	void rlock_np();
	void wlock();
	void wlock_np();

	void un_rlock();
	void un_rlock_np();
	void un_wlock();
	void un_wlock_np();
};


class spin_rlock_section
{
	DISALLOW_COPY_AND_ASSIGN(spin_rlock_section);

public:
	spin_rlock_section(spin_rwlock& lock) :
		_lock(lock)
	{
		_lock.rlock();
	}
	~spin_rlock_section() {
		_lock.un_rlock();
	}

private:
	spin_rwlock& _lock;
};

class spin_rlock_section_np
{
	DISALLOW_COPY_AND_ASSIGN(spin_rlock_section_np);

public:
	spin_rlock_section_np(spin_rwlock& lock) :
		_lock(lock)
	{
		_lock.rlock_np();
	}
	~spin_rlock_section_np()
	{
		_lock.un_rlock_np();
	}

private:
	spin_rwlock& _lock;
};

class spin_wlock_section
{
	DISALLOW_COPY_AND_ASSIGN(spin_wlock_section);

public:
	spin_wlock_section(spin_rwlock& lock) :
		_lock(lock)
	{
		_lock.wlock();
	}
	~spin_wlock_section() {
		_lock.un_wlock();
	}

private:
	spin_rwlock& _lock;
};

class spin_wlock_section_np
{
	DISALLOW_COPY_AND_ASSIGN(spin_wlock_section_np);

public:
	spin_wlock_section_np(spin_rwlock& lock) :
		_lock(lock)
	{
		_lock.wlock_np();
	}
	~spin_wlock_section_np()
	{
		_lock.un_wlock_np();
	}

private:
	spin_rwlock& _lock;
};


#endif  // include guard

