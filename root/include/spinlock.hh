/// @file spinlock.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_SPINLOCK_HH_
#define INCLUDE_SPINLOCK_HH_

#include <atomic.hh>
#include <cpu_node.hh>
#include <spinlock_ops.hh>


class spin_lock
{
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
	bool try_lock();

	void unlock();

private:
	bool _try_lock() {
		return atom.exchange(LOCKED) == UNLOCKED;
	}
};


class spin_rwlock : public arch::spin_rwlock_ops
{
public:
	bool try_rlock();
	bool try_wlock();

	void rlock();
	void wlock();

	void unlock();
};


#endif  // include guard

