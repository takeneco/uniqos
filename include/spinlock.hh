/// @file spinlock.hh
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_SPINLOCK_HH_
#define INCLUDE_SPINLOCK_HH_

#include <atomic.hh>
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

