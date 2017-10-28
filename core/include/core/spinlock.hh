/// @file core/spinlock.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

	spin_rwlock& operator () () { return *this; }

	bool try_rlock();
	bool try_wlock();

	void rlock(bool np) { np ? rlock_np() : rlock(); }
	void rlock();
	void rlock_np();
	void wlock(bool np) { np ? wlock_np() : wlock(); }
	void wlock();
	void wlock_np();

	void un_rlock(bool np) { np ? un_rlock_np() : un_rlock(); }
	void un_rlock();
	void un_rlock_np();
	void un_wlock(bool np) { np ? un_wlock_np() : un_wlock(); }
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

/** @defgroup spin_locked
 * @{
 * @brief Read locked or write locked return value holder classes.
 * - spin_rlocked will be rlock() on constructor and un_rlock() on destructor.
 * - spin_wlocked will be wlock() on constructor and un_wlock() on destructor.
 * The TYPE must have a TYPE::self_lock member, which is instance of
 * spin_rwlock or getter of spin_rwlock.
 *
 * - spin_rlocked_pair is same as spin_rlocked but spin_rwlock is out of member.
 * - spin_wlocked_pair is same as spin_wlocked but spin_rwlock is out of member.
 *
   @code
   struct X1 {
       spin_rwlock self_lock;
   } x1;
   spin_rlocked<X1> get_r_X1() {
       // x1.self_lock will be rlock() by constructor.
       return create_spin_rlocked(&x1);
   }
   spin_wlocked<X2> get_w_X1() {
       // x1.self_lock will be wlock() by constructor.
       return create_spin_wlocked(&x1);
   }
  
   void func1_r() {
       auto x = get_r_X1();
       // x.value()->self_lock will be un_rlock() by destructor.
   }
   void func1_w() {
       auto x = get_w_X2();
       // x.value()->self_lock will be un_wlock() by destructor.
   }
   @endcode

   @code
   struct X2 {} x2;
   spin_rwlock x2_lock;
   spin_rlocked_pair<X2> get_w_X2() {
       // x2_lock will be rlock() by constructor.
       return create_spin_rlocked_pair(&x2, &x2_lock);
   }
   spin_rlocked_pair<X2> get_w_X2() {
       // x2_lock will be wlock() by constructor.
       return create_spin_wlocked_pair(&x2, &x2_lock);
   }
  
   void func2_r() {
       auto x = get_w_X2();
       // x2_lock will be un_rlock() by destructor.
   }
   void func2_w() {
       auto x = get_w_X2();
       // x2_lock will be un_wlock() by destructor.
   }
   @endcode
 */
template<class TYPE, bool NP=false>
class spin_rlocked
{
	DISALLOW_COPY_AND_ASSIGN(spin_rlocked);

public:
	spin_rlocked(TYPE* value) :
		_value(value)
	{
		_rlock();
	}

	spin_rlocked(spin_rlocked&& other) :
		_value(other._value)
	{
		other._value = nullptr;
	}

	~spin_rlocked()
	{
		_un_rlock();
	}

	TYPE* value() { return _value; }
	operator TYPE* () { return _value; }
	TYPE* operator -> () { return _value; }

	void un_rlock()
	{
		_un_rlock();
		_value = nullptr;
	}

private:
	void _rlock()
	{
		if (_value) {
			if (NP)
				_value->self_lock().rlock_np();
			else
				_value->self_lock().rlock();
		}
	}
	void _un_rlock() {
		if (_value)
			_value->self_lock().un_rlock(NP);
	}

private:
	TYPE* _value;
};

/// @brief Creates spin_rlocked instance.
template<class TYPE, bool NP=false>
inline
spin_rlocked<TYPE, NP>
create_spin_rlocked(TYPE* value)
{
	return spin_rlocked<TYPE, NP>(value);
}

template<class TYPE, bool NP=false>
class spin_wlocked
{
	DISALLOW_COPY_AND_ASSIGN(spin_wlocked);

public:
	spin_wlocked(TYPE* value) :
		_value(value)
	{
		_wlock();
	}

	spin_wlocked(spin_wlocked&& other) :
		_value(other._value)
	{
		other._value = nullptr;
	}

	~spin_wlocked()
	{
		_un_wlock();
		_value = nullptr;
	}

	TYPE* value() { return _value; }
	operator TYPE* () { return _value; }
	TYPE* operator -> () { return _value; }

	void un_wlock()
	{
		_un_wlock();
		_value = nullptr;
	}

private:
	void _wlock() {
		if (_value)
			_value->self_lock().wlock(NP);
	}
	void _un_wlock() {
		if (_value)
			_value->self_lock().un_wlock(NP);
	}

private:
	TYPE* _value;
};

/// @brief Creates spin_wlocked instance.
template<class TYPE, bool NP=false>
inline
spin_wlocked<TYPE, NP>
create_spin_wlocked(TYPE* value)
{
	return spin_wlocked<TYPE, NP>(value);
}

template<class TYPE, bool NP=false>
class spin_rlocked_pair
{
	DISALLOW_COPY_AND_ASSIGN(spin_rlocked_pair);

public:
	spin_rlocked_pair(TYPE* value, spin_rwlock* lock) :
		_value(value),
		_lock(lock)
	{
		_rlock();
	}

	spin_rlocked_pair(spin_rlocked_pair&& other) :
		_value(other._value),
		_lock(other._lock)
	{
		other._value = nullptr;
	}

	~spin_rlocked_pair()
	{
		_un_rlock();
	}

	TYPE* value() { return _value; }
	operator TYPE* () { return _value; }
	TYPE* operator -> () { return _value; }

	void un_rlock()
	{
		_un_rlock();
		_value = nullptr;
	}

private:
	void _rlock() {
		if (_value)
			_lock->rlock(NP);
	}
	void _un_rlock() {
		if (_value)
			_lock->un_rlock(NP);
	}

private:
	TYPE* _value;
	spin_rwlock* _lock;
};

/// @brief Creates spin_rlocked_pair instance.
template<class TYPE, bool NP=false>
inline
spin_rlocked_pair<TYPE, NP>
create_spin_rlocked_pair(TYPE* value, spin_rwlock* lock)
{
	return spin_rlocked_pair<TYPE, NP>(value, lock);
}

template<class TYPE, bool NP=false>
class spin_wlocked_pair
{
	DISALLOW_COPY_AND_ASSIGN(spin_wlocked_pair);

public:
	spin_wlocked_pair(TYPE* value, spin_rwlock* lock) :
		_value(value),
		_lock(lock)
	{
		_wlock();
	}

	spin_wlocked_pair(spin_wlocked_pair&& other) :
		_value(other._value),
		_lock(other._lock)
	{
		other._value = nullptr;
	}

	~spin_wlocked_pair()
	{
		_un_wlock();
	}

	TYPE* value() { return _value; }
	operator TYPE* () { return _value; }
	TYPE* operator -> () { return _value; }

	void un_wlock()
	{
		_un_wlock();
		_value = nullptr;
	}

private:
	void _wlock() {
		if (_value)
			_lock->wlock(NP);
	}
	void _un_wlock() {
		if (_value)
			_lock->un_wlock(NP);
	}

private:
	TYPE* _value;
	spin_rwlock* _lock;
};

/// @brief Creates spin_wlocked_pair instance.
template<class TYPE, bool NP=false>
inline
spin_wlocked_pair<TYPE, NP>
create_spin_wlocked_pair(TYPE* value, spin_rwlock* lock)
{
	return spin_wlocked_pair<TYPE, NP>(value, lock);
}

/// @}


#endif  // CORE_SPINLOCK_HH_

