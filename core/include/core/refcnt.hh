//  Uniqos  --  Unique Operating System
//  (C) 2017 KATO Takeshi
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

#ifndef CORE_REFCNT_HH_
#define CORE_REFCNT_HH_

#include <core/basic-types.hh>
#include <util/atomic.hh>


/// @brief Reference counter
template<class CNT=harf_of<uptr>::t>
class refcnt
{
public:
	refcnt() : cnt(0) {}
	CNT get() { cnt.load(); }
	void inc() { cnt.inc(); }
	void dec() { cnt.dec(); }

protected:
	atomic<CNT> cnt;
};

template<class T>
class dec_on_dtor
{
public:
	dec_on_dtor(T* p) :
		ptr(p)
	{}
	~dec_on_dtor()
	{
		if (ptr)
			ptr->refs.dec();
	}

	operator T* () {
		return ptr;
	}
	T* operator -> () {
		return ptr;
	}

	void release() {
		ptr = nullptr;
	}

private:
	T* ptr;
};


#endif  // CORE_REFCNT_HH_

