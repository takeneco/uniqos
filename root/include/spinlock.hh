/// @file spinlock.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_SPINLOCK_HH_
#define INCLUDE_SPINLOCK_HH_

#include <atomic.hh>
#include <spinlock_ops.hh>


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
	spin_lock_section(spin_lock& lock, bool no_preempt = false) :
		_lock(lock),
		_no_preempt(no_preempt)
	{
		if (_no_preempt)
			_lock.lock_np();
		else
			_lock.lock();
	}
	~spin_lock_section() {
		if (_no_preempt)
			_lock.unlock_np();
		else
			_lock.unlock();
	}

private:
	spin_lock& _lock;
	const bool _no_preempt;
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
	spin_rlock_section(spin_rwlock& lock, bool no_preempt = false) :
		_lock(lock),
		_no_preempt(no_preempt)
	{
		if (_no_preempt)
			_lock.rlock_np();
		else
			_lock.rlock();
	}
	~spin_rlock_section() {
		if (_no_preempt)
			_lock.un_rlock_np();
		else
			_lock.un_rlock();
	}

private:
	spin_rwlock& _lock;
	const bool _no_preempt;
};


class spin_wlock_section
{
	DISALLOW_COPY_AND_ASSIGN(spin_wlock_section);

public:
	spin_wlock_section(spin_rwlock& lock, bool no_preempt = false) :
		_lock(lock),
		_no_preempt(no_preempt)
	{
		if (_no_preempt)
			_lock.wlock_np();
		else
			_lock.wlock();
	}
	~spin_wlock_section() {
		if (_no_preempt)
			_lock.un_wlock_np();
		else
			_lock.un_wlock();
	}

private:
	spin_rwlock& _lock;
	const bool _no_preempt;
};


#endif  // include guard

