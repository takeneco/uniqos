/// @file  chain.hh
/// @brief Link list structure.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef CHAIN_HH_
#define CHAIN_HH_


/// @brief  Directional chain.
template<class DATA>
class chain_link
{
	template<class A, class B, class C, B& (A::* D)()>
	friend class chain_impl_;

	DATA* next;

	void set_prev_(DATA*) { /* nothing */ }
	void set_next_(DATA* p) { next = p; }

public:
	const DATA* get_next() const { return next; }
	      DATA* get_next()       { return next; }
};

/// @brief  Bidirectional chain.
template<class DATA>
class bichain_link : public chain_link<DATA>
{
	template<class A, class B, class C, B& (A::* D)()>
	friend class chain_impl_;

	DATA* prev;

	void set_prev_(DATA* p) { prev = p; }

public:
	const DATA* get_prev() const { return prev; }
	      DATA* get_prev()       { return prev; }
};

/// @brief  Single ended chain.
template<class DATA>
class chain_end_
{
	DATA* head;

public:
	chain_end_() : head(0) {}

	void set_head(DATA* p) { head = p; }
	void set_tail(DATA*) { /* nothing */ }
	const DATA* get_head() const { return head; }
	      DATA* get_head()       { return head; }
	static DATA* null() { return 0; }
};

/// @brief  Double ended chain.
template<class DATA>
class dechain_end_ : public chain_end_<DATA>
{
	DATA* tail;

public:
	dechain_end_() : chain_end_<DATA>(), tail(0) {}

	void set_tail(DATA* p) { tail = p; }
	const DATA* get_tail() const { return tail; }
	      DATA* get_tail()       { return tail; }
};

/// @brief  Chain implemnts.
template<
    class DATA,
    class LINK_TYPE,
    class END,
    LINK_TYPE& (DATA::* LINK)()>
class chain_impl_
{
	static void set_prev(DATA* p, DATA* prev) {
		(p->*LINK)().set_prev_(prev); }
	static void set_next(DATA* p, DATA* next) {
		(p->*LINK)().set_next_(next); }

protected:
	END end;

public:
	chain_impl_() : end() {}

	// 1end 2end & dir, bidir
	const DATA* head() const { return end.get_head(); }
	      DATA* head()       { return end.get_head(); }

	// 2end & dir, bidir
	const DATA* tail() const { return end.get_tail(); }
	      DATA* tail()       { return end.get_tail(); }

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
		if (head() != end.null())
			set_prev(head(), p);
		else
			end.set_tail(p);
		end.set_head(p);
	}
	// 2end & dir, bidir
	void insert_tail(DATA* p) {
		set_prev(p, tail());
		set_next(p, 0);
		if (head() != end.null())
			set_next(tail(), p);
		else
			end.set_head(p);
		end.set_tail(p);
	}
	// 1end, 2end & bidir
	void insert_prev(DATA* base, DATA* p) {
		set_prev(p, prev(base));
		set_next(p, base);
		if (prev(base))
			set_next(prev(base), p);
		else
			end.set_head(p);
		set_prev(base, p);
	}
	// 1end, 2end & dir, bidir
	void insert_next(DATA* base, DATA* p) {
		set_prev(p, base);
		set_next(p, next(base));
		if (next(base))
			set_prev(next(base), p);
		else
			end.set_tail(p);
		set_next(base, p);
	}
	// 1end, 2end & dir, bidir
	DATA* remove_head() {
		DATA* h = head();
		if (h) {
			DATA* h2 = next(h);
			end.set_head(h2);
			if (h2 != end.null())
				set_prev(h2, 0);
			else
				end.set_tail(0);
		}
		return h;
	}
	// 2end & bidir
	DATA* remove_tail() {
		DATA* t = tail();
		if (t) {
			DATA* t2 = prev(t);
			end.set_tail(t2);
			if (t2 != end.null())
				set_next(t2, 0);
			else
				end.set_head(0);
		}
		return t;
	}
	// 1end, 2end & bidir
	void remove(DATA* p) {
		if (prev(p))
			set_next(prev(p), next(p));
		else
			end.set_head(next(p));
		if (next(p))
			set_prev(next(p), prev(p));
		else
			end.set_tail(prev(p));
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
				end.set_tail(p);
		//}
	}
};

/// Directional single ended chain
template<class DATA, chain_link<DATA>& (DATA::* LINK)()>
class chain : public chain_impl_
    <DATA, chain_link<DATA>, chain_end_<DATA>, LINK>
{
};

/// Bidirectional and single ended chain
template<class DATA, bichain_link<DATA>& (DATA::* LINK)()>
class bichain : public chain_impl_
    <DATA, bichain_link<DATA>, chain_end_<DATA>, LINK>
{
};

/// Directional and double ended chain
template<class DATA, chain_link<DATA>& (DATA::* LINK)()>
class dechain : public chain_impl_
    <DATA, chain_link<DATA>, dechain_end_<DATA>, LINK>
{
};

/// Bidirectional and double ended chain
template<class DATA, bichain_link<DATA>& (DATA::* LINK)()>
class bidechain : public chain_impl_
    <DATA, bichain_link<DATA>, dechain_end_<DATA>, LINK>
{
};


#endif  // include guard
