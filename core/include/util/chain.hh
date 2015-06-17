/// @file  util/chain.hh
/// @brief Link list structure.

//  Uniqos  --  Unique Operating System
//  (C) 2010-2015 KATO Takeshi
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

#ifndef UTIL_CHAIN_HH_
#define UTIL_CHAIN_HH_


/// @brief  Directional chain node.
template<class DATA>
class forward_chain_node
{
	template<class A, class B, class C, class D, D E>
	friend class chain_impl_;

	DATA* next;

	const forward_chain_node<DATA>& operator () () const { return *this; }
	      forward_chain_node<DATA>& operator () ()       { return *this; }

	void _set_prev(DATA*) { /* nothing */ }
	void _set_next(DATA* p) { next = p; }

public:
	const DATA* get_next() const { return next; }
	      DATA* get_next()       { return next; }
};

/// @brief  Bidirectional chain node.
template<class DATA>
class chain_node : public forward_chain_node<DATA>
{
	template<class A, class B, class C, class D, D E>
	friend class chain_impl_;

	DATA* prev;

	const chain_node<DATA>& operator () () const { return *this; }
	      chain_node<DATA>& operator () ()       { return *this; }

	void _set_prev(DATA* p) { prev = p; }

public:
	const DATA* get_prev() const { return prev; }
	      DATA* get_prev()       { return prev; }
};

/// @brief  Single side chain.
template<class DATA>
class front_chain_end_
{
	DATA* front;

public:
	front_chain_end_() :
		front(0)
	{}

	void set_front(DATA* p) { front = p; }
	void set_back(DATA*) { /* nothing */ }
	const DATA* get_front() const { return front; }
	      DATA* get_front()       { return front; }
	static DATA* null() { return 0; }
};

/// @brief  Double side chain.
template<class DATA>
class back_chain_end_ : public front_chain_end_<DATA>
{
	DATA* back;

public:
	back_chain_end_() :
		front_chain_end_<DATA>(),
		back(0)
	{}

	void set_back(DATA* p) { back = p; }
	const DATA* get_back() const { return back; }
	      DATA* get_back()       { return back; }
};

/// @brief  Chain implemnts.
template<
    class DATA,
    class LINK_TYPE,
    class END,
    class LINK_STYLE,
    LINK_STYLE LINK>
class chain_impl_
{
	typedef chain_impl_<DATA, LINK_TYPE, END, LINK_STYLE, LINK> self_t;

	static void set_prev(DATA* p, DATA* prev) {
		(p->*LINK)()._set_prev(prev); }
	static void set_next(DATA* p, DATA* next) {
		(p->*LINK)()._set_next(next); }

protected:
	END chain_end;

public:
	using data_t = DATA;

	class iterator
	{
		DATA* data;
		LINK_TYPE& node() {
			return (data->*LINK)();
		}

	public:
		iterator(DATA* _data) : data(_data) {}
		iterator(const iterator& it) : data(it.data) {}
		~iterator() {}

	public:
		DATA* operator * () {
			return data;
		}
		DATA* operator -> () {
			return data;
		}
		iterator& operator = (iterator& it) {
			data = it.data;
			return *this;
		}
		bool operator == (const iterator& it) const {
			return data == it.data;
		}
		bool operator != (const iterator& it) const {
			return data != it.data;
		}
		iterator& operator ++ () {
			data = node().get_next();
			return *this;
		}
		iterator operator ++ (int) {
			iterator tmp(data);
			data = node().get_next();
			return tmp;
		}
		iterator& operator -- () {
			data = node().get_prev();
			return *this;
		}
		iterator operator -- (int) {
			iterator tmp(data);
			data = node().get_prev();
			return tmp;
		}

	public:
		iterator next() {
			return iterator(data->get_next());
		}
		iterator prev() {
			return iterator(data->get_prev());
		}

	public:
		DATA* ref() {
			return data;
		}
	};

	class const_iterator
	{
		const DATA* data;
		const LINK_TYPE& node() {
			return (const_cast<DATA*>(data)->*LINK)();
		}

	public:
		const_iterator(const DATA* _data) : data(_data) {}
		const_iterator(const const_iterator& it) : data(it.data) {}
		~const_iterator() {}

	public:
		const DATA* operator * () const {
			return data;
		}
		const DATA* operator -> () const {
			return data;
		}
		const_iterator& operator = (const_iterator& it) {
			data = it.data;
			return *this;
		}
		bool operator == (const const_iterator& it) const {
			return data == it.data;
		}
		bool operator != (const const_iterator& it) const {
			return data != it.data;
		}
		const_iterator& operator ++ () {
			data = node().get_next();
			return *this;
		}
		const_iterator operator ++ (int) {
			const_iterator tmp(data);
			data = node().get_next();
			return tmp;
		}
		const_iterator& operator -- () {
			data = node().get_prev();
			return *this;
		}
		const_iterator operator -- (int) {
			const_iterator tmp(data);
			data = node().get_prev();
			return tmp;
		}

	public:
		const_iterator next() const {
			return const_iterator(data->get_next());
		}
		const_iterator prev() const {
			return const_iterator(data->get_prev());
		}

	public:
		const DATA* ref() const {
			return data;
		}
	};

	chain_impl_() :
		chain_end()
	{}

	iterator begin() {
		return iterator(chain_end.get_front());
	}
	const_iterator begin() const {
		return const_iterator(chain_end.get_front());
	}

	iterator end() {
		return iterator(nullptr);
	}
	const_iterator end() const {
		return const_iterator(nullptr);
	}

	// any
	const DATA* front() const { return chain_end.get_front(); }
	      DATA* front()       { return chain_end.get_front(); }

	// !front_chain !front_forward_chain
	const DATA* back() const { return chain_end.get_back(); }
	      DATA* back()       { return chain_end.get_back(); }

	bool is_empty() const { return chain_end.get_front() == 0; }

	void move_to(self_t* to) {
		to->chain_end.set_front(chain_end.get_front());
		to->chain_end.set_back(chain_end.get_back());
		chain_end.set_front(nullptr);
		chain_end.set_back(nullptr);
	}

	// !forward_chain !front_froward_chain
	static const DATA* prev(const DATA* p) {
		return (const_cast<DATA*>(p)->*LINK)().prev;
	}
	static       DATA* prev(DATA* p) {
		return (p->*LINK)().prev;
	}

	// 1end, 2end & dir, bidir
	static const DATA* next(const DATA* p) {
		return (const_cast<DATA*>(p)->*LINK)().next; }
	static       DATA* next(DATA* p) {
		return (p->*LINK)().next; }

	// any
	void push_front(DATA* p) {
		set_prev(p, 0);
		set_next(p, front());
		if (front() != chain_end.null())
			set_prev(front(), p);
		else
			chain_end.set_back(p);
		chain_end.set_front(p);
	}
	// !front_chain !front_forward_chain
	void push_back(DATA* p) {
		set_prev(p, back());
		set_next(p, 0);
		if (front() != chain_end.null())
			set_next(back(), p);
		else
			chain_end.set_front(p);
		chain_end.set_back(p);
	}
	// !forward_chain !front_forward_chain
	void insert_before(DATA* base, DATA* p) {
		set_prev(p, prev(base));
		set_next(p, base);
		if (prev(base))
			set_next(prev(base), p);
		else
			chain_end.set_front(p);
		set_prev(base, p);
	}
	// any
	void insert_after(DATA* base, DATA* p) {
		set_prev(p, base);
		set_next(p, next(base));
		if (next(base))
			set_prev(next(base), p);
		else
			chain_end.set_back(p);
		set_next(base, p);
	}
	// any
	DATA* pop_front() {
		DATA* h = front();
		if (h) {
			DATA* h2 = next(h);
			chain_end.set_front(h2);
			if (h2 != chain_end.null())
				set_prev(h2, 0);
			else
				chain_end.set_back(0);
		}
		return h;
	}
	// chain only
	DATA* pop_back() {
		DATA* t = back();
		if (t) {
			DATA* t2 = prev(t);
			chain_end.set_back(t2);
			if (t2 != chain_end.null())
				set_next(t2, 0);
			else
				chain_end.set_front(0);
		}
		return t;
	}
	// 1end, 2end & bidir
	void remove(DATA* p) {
		if (prev(p))
			set_next(prev(p), next(p));
		else
			chain_end.set_front(next(p));
		if (next(p))
			set_prev(next(p), prev(p));
		else
			chain_end.set_back(prev(p));
	}
	// 1end, 2end & dir, bidir
	void remove_next(DATA* p) {
		DATA* next1 = next(p);
		if (next1) {
			DATA* next2 = next(next1);
			set_next(p, next2);
			if (next2)
				set_prev(next2, p);
			else
				chain_end.set_back(p);
		}
	}
};

/// Directional one side chain
// flexible
template<class DATA, forward_chain_node<DATA>& (DATA::* LINK)()>
using front_forward_fchain = chain_impl_<
    DATA,
    forward_chain_node<DATA>,
    front_chain_end_<DATA>,
    forward_chain_node<DATA>& (DATA::*)(),
    LINK>;

// unflexible
template<class DATA, forward_chain_node<DATA> DATA::* LINK>
using front_forward_chain = chain_impl_<
    DATA,
    forward_chain_node<DATA>,
    front_chain_end_<DATA>,
    forward_chain_node<DATA> DATA::*,
    LINK>;


/// Bidirectional one side chain
// flexible
template<class DATA, chain_node<DATA>& (DATA::* LINK)()>
using front_fchain = chain_impl_<
    DATA,
    chain_node<DATA>,
    front_chain_end_<DATA>,
    chain_node<DATA>& (DATA::*)(),
    LINK>;

// unflexible
template<class DATA, chain_node<DATA> DATA::* LINK>
using front_chain = chain_impl_<
    DATA,
    chain_node<DATA>,
    front_chain_end_<DATA>,
    chain_node<DATA> DATA::*,
    LINK>;


/// Directional both side chain
// flexible
template<class DATA, forward_chain_node<DATA>& (DATA::* LINK)()>
using forward_fchain = chain_impl_<
    DATA,
    forward_chain_node<DATA>,
    back_chain_end_<DATA>,
    forward_chain_node<DATA>& (DATA::*)(),
    LINK>;

// unflexible
template<class DATA, forward_chain_node<DATA> DATA::* LINK>
using forward_chain = chain_impl_<
    DATA,
    forward_chain_node<DATA>,
    back_chain_end_<DATA>,
    forward_chain_node<DATA> DATA::*,
    LINK>;


/// Bidirectional both side chain
// flexible
template<class DATA, chain_node<DATA>& (DATA::* LINK)()>
using fchain = chain_impl_<
    DATA,
    chain_node<DATA>,
    back_chain_end_<DATA>,
    chain_node<DATA>& (DATA::*)(),
    LINK>;

// unflexible
template<class DATA, chain_node<DATA> DATA::* LINK>
using chain = chain_impl_<
    DATA,
    chain_node<DATA>,
    back_chain_end_<DATA>,
    chain_node<DATA> DATA::*,
    LINK>;


#endif  // include guard

