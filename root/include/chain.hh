/// @file  chain.hh
/// @brief Link list structure.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef CHAIN_HH_
#define CHAIN_HH_


/// @brief  Directional chain node.
template<class DATA>
class chain_node
{
	template<class A, class B, class C, B& (A::* D)()>
	friend class chain_impl_;

	DATA* next;

	void _set_prev(DATA*) { /* nothing */ }
	void _set_next(DATA* p) { next = p; }

public:
	const DATA* get_next() const { return next; }
	      DATA* get_next()       { return next; }
};

/// @brief  Bidirectional chain node.
template<class DATA>
class bichain_node : public chain_node<DATA>
{
	template<class A, class B, class C, B& (A::* D)()>
	friend class chain_impl_;

	DATA* prev;

	void _set_prev(DATA* p) { prev = p; }

public:
	const DATA* get_prev() const { return prev; }
	      DATA* get_prev()       { return prev; }
};

/// @brief  Single side chain.
template<class DATA>
class chain_side_
{
	DATA* front;

public:
	chain_side_() : front(0) {}

	void set_front(DATA* p) { front = p; }
	void set_back(DATA*) { /* nothing */ }
	const DATA* get_front() const { return front; }
	      DATA* get_front()       { return front; }
	static DATA* null() { return 0; }
};

/// @brief  Double side chain.
template<class DATA>
class dechain_side_ : public chain_side_<DATA>
{
	DATA* back;

public:
	dechain_side_() : chain_side_<DATA>(), back(0) {}

	void set_back(DATA* p) { back = p; }
	const DATA* get_back() const { return back; }
	      DATA* get_back()       { return back; }
};

/// @brief  Chain implemnts.
template<
    class DATA,
    class LINK_TYPE,
    class SIDE,
    LINK_TYPE& (DATA::* LINK)()>
class chain_impl_
{
	static void set_prev(DATA* p, DATA* prev) {
		(p->*LINK)()._set_prev(prev); }
	static void set_next(DATA* p, DATA* next) {
		(p->*LINK)()._set_next(next); }

protected:
	SIDE side;

public:
	class iterator
	{
		DATA* data;
	public:
		iterator(DATA* _data) : data(_data) {}
		iterator(iterator& it) : data(it.data) {}
		~iterator() {}

		iterator& operator = (iterator& it) {
			data = it.data;
		}
		iterator& operator ++ () {
			data = data->get_next();
			return *this;
		}
		iterator operator ++ (int) {
			iterator tmp(data);
			data = data->get_next();
			return tmp;
		}
		iterator& operator -- () {
			data = data->get_prev();
			return *this;
		}
		iterator operator -- (int) {
			iterator tmp(data);
			data = data->get_prev();
			return tmp;
		}
	};

	chain_impl_() : side() {}

	// 1end 2end & dir, bidir
	const DATA* head() const { return side.get_front(); }
	      DATA* head()       { return side.get_front(); }

	const DATA* front() const { return side.get_front(); }
	      DATA* front()       { return side.get_front(); }

	iterator begin() { return iterator(side.get_front()); }

	// 2end & dir, bidir
	const DATA* tail() const { return side.get_back(); }
	      DATA* tail()       { return side.get_back(); }

	const DATA* back() const { return side.get_back(); }
	      DATA* back()       { return side.get_back(); }

	iterator end() { return iterator(0); }

	// 1end, 2end & bidir
	static const DATA* prev(const DATA* p) {
		return (const_cast<DATA*>(p)->*LINK)().prev; }
	static       DATA* prev(DATA* p) {
		return (p->*LINK)().prev; }

	// 1end, 2end & dir, bidir
	static const DATA* next(const DATA* p) {
		return (const_cast<DATA*>(p)->*LINK)().next; }
	static       DATA* next(DATA* p) {
		return (p->*LINK)().next; }

	// 1end, 2end & dir, bidir
	void insert_head(DATA* p) {
		set_prev(p, 0);
		set_next(p, head());
		if (head() != side.null())
			set_prev(head(), p);
		else
			side.set_back(p);
		side.set_front(p);
	}
	// 2end & dir, bidir
	void insert_tail(DATA* p) {
		set_prev(p, tail());
		set_next(p, 0);
		if (head() != side.null())
			set_next(tail(), p);
		else
			side.set_front(p);
		side.set_back(p);
	}
	// 1end, 2end & bidir
	void insert_prev(DATA* base, DATA* p) {
		set_prev(p, prev(base));
		set_next(p, base);
		if (prev(base))
			set_next(prev(base), p);
		else
			side.set_front(p);
		set_prev(base, p);
	}
	// 1end, 2end & dir, bidir
	void insert_next(DATA* base, DATA* p) {
		set_prev(p, base);
		set_next(p, next(base));
		if (next(base))
			set_prev(next(base), p);
		else
			side.set_back(p);
		set_next(base, p);
	}
	// 1end, 2end & dir, bidir
	DATA* remove_head() {
		DATA* h = head();
		if (h) {
			DATA* h2 = next(h);
			side.set_front(h2);
			if (h2 != side.null())
				set_prev(h2, 0);
			else
				side.set_back(0);
		}
		return h;
	}
	// 2end & bidir
	DATA* remove_tail() {
		DATA* t = tail();
		if (t) {
			DATA* t2 = prev(t);
			side.set_back(t2);
			if (t2 != side.null())
				set_next(t2, 0);
			else
				side.set_front(0);
		}
		return t;
	}
	// 1end, 2end & bidir
	void remove(DATA* p) {
		if (prev(p))
			set_next(prev(p), next(p));
		else
			side.set_front(next(p));
		if (next(p))
			set_prev(next(p), prev(p));
		else
			side.set_back(prev(p));
	}
	// 1end, 2end & dir, bidir
	void remove_next(DATA* p) {
		DATA* next = next(p);
		//if (next) {
			DATA* nnext = next(next);
			set_next(p, nnext);
			if (nnext)
				set_prev(nnext, p);
			else
				side.set_back(p);
		//}
	}
};

/// Directional one side chain
template<class DATA, chain_node<DATA>& (DATA::* LINK)()>
class chain : public chain_impl_
    <DATA, chain_node<DATA>, chain_side_<DATA>, LINK>
{
};

/// Bidirectional one side chain
template<class DATA, bichain_node<DATA>& (DATA::* LINK)()>
class bichain : public chain_impl_
    <DATA, bichain_node<DATA>, chain_side_<DATA>, LINK>
{
};

/// Directional both side chain
template<class DATA, chain_node<DATA>& (DATA::* LINK)()>
class bochain : public chain_impl_
    <DATA, chain_node<DATA>, dechain_side_<DATA>, LINK>
{
};

/// Bidirectional both side chain
template<class DATA, bichain_node<DATA>& (DATA::* LINK)()>
class bibochain : public chain_impl_
    <DATA, bichain_node<DATA>, dechain_side_<DATA>, LINK>
{
};


#endif  // include guard
