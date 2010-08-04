// @file    chain.hh
// @author  Kato Takeshi
// @brief   List structure.
//
// (C) 2010 Kato Takeshi.

#ifndef CHAIN_HH_
#define CHAIN_HH_


// @brief  Directional chain.
template<class DataType>
class chain_link
{
	template<class A, class B, class C, B A::* D> friend class _chain_impl;
	DataType* next;

public:
	void _set_prev(DataType* p) {}
	void _set_next(DataType* p) { next = p; }
	const DataType* get_next() const { return next; }
	      DataType* get_next()       { return next; }
};

// @brief  Bidirectional chain.
template<class DataType>
class bichain_link : public chain_link<DataType>
{
	template<class A, class B, class C, B A::* D> friend class _chain_impl;
	DataType* prev;

public:
	void _set_prev(DataType* p) { prev = p; }
	const DataType* get_prev() const { return prev; }
	      DataType* get_prev()       { return prev; }
};

// @brief  Single ended chain.
template<class DataType>
class _chain_end
{
public:
	DataType* head;
	_chain_end() : head(0) {}
	void set_head(DataType* p) { head = p; }
	void set_tail(DataType* p) {}
};

// @brief  Double ended chain.
template<class DataType>
class _dechain_end : public _chain_end<DataType>
{
public:
	DataType* tail;
	_dechain_end() : _chain_end<DataType>(), tail(0) {}

	void set_tail(DataType* p) { tail = p; }
};

// @brief  Chain implemnts.
template<class DataType, class LinkType, class EndType, LinkType DataType::* LinkVal>
class _chain_impl
{
	EndType end;

	const DataType& prev(const DataType* p) const { return *(p->*LinkVal).prev; }
	      DataType& prev(DataType* p)             { return *(p->*LinkVal).prev; }
	const DataType& next(const DataType* p) const { return *(p->*LinkVal).next; }
	      DataType& next(DataType* p)             { return *(p->*LinkVal).next; }

	void set_prev(DataType* p, DataType* prev) { (p->*LinkVal)._set_prev(prev); }
	void set_next(DataType* p, DataType* next) { (p->*LinkVal)._set_next(next); }
public:
	_chain_impl() : end() {}

	const DataType* get_head() const { return end.head; }
	      DataType* get_head()       { return end.head; }
	const DataType* get_tail() const { return end.tail; }
	      DataType* get_tail()       { return end.tail; }
	const DataType* get_prev(const DataType* p) const { return (p->*LinkVal).prev; }
	      DataType* get_prev(DataType* p)             { return (p->*LinkVal).prev; }
	const DataType* get_next(const DataType* p) const { return (p->*LinkVal).next; }
	      DataType* get_next(DataType* p)             { return (p->*LinkVal).next; }

	void insert_head(DataType* p) {
		set_prev(p, 0);
		set_next(p, end.head);
		if (end.head)
			set_prev(end.head, p);
		else
			end.set_tail(p);
		end.set_head(p);
	}
	void insert_tail(DataType* p) {
		set_prev(p, end.tail);
		set_next(p, 0);
		if (end.tail)
			set_next(end.tail, p);
		else
			end.set_head(p);
		end.set_tail(p);
	}
	void insert_prev(DataType* base, DataType* p) {
		set_prev(p, get_prev(base));
		set_next(p, base);
		if (get_prev(base))
			set_next(get_prev(base), p);
		else
			end.set_head(p);
		set_prev(base, p);
	}
	void insert_next(DataType* base, DataType* p) {
		set_prev(p, base);
		set_next(p, get_next(base));
		if (get_next(base))
			set_prev(get_next(base), p);
		else
			end.set_tail(p);
		set_next(base, p);
	}
	DataType* remove_head() {
		DataType* head = end.head;
		if (head) {
			end.set_head(get_next(head));
			if (end.head)
				set_prev(end.head, 0);
			else
				end.set_tail(0);
		}
		return head;
	}
	DataType* remove_tail() {
		DataType* tail = end.tail;
		if (tail) {
			end.set_tail(get_prev(tail));
			if (end.tail)
				set_next(end.tail, 0);
			else
				end.set_head(0);
		}
		return tail;
	}
	DataType* remove(DataType* p) {
		if (get_prev(p))
			set_next(get_prev(p), get_next(p));
		else
			end.set_head(get_next(p));
		if (get_next(p))
			set_prev(get_next(p), get_prev(p));
		else
			end.set_tail(get_prev(p));
	}
	DataType* remove_next(DataType* p) {
		DataType* next = get_next(p);
		//if (next) {
			DataType* nnext = get_next(next);
			set_next(p, nnext);
			if (nnext)
				set_prev(nnext, p);
			else
				end.set_tail(p);
		//}
	}
};

/// Directional single ended chain
template<class DataType, chain_link<DataType> DataType::* LinkVal>
class chain : public _chain_impl
    <DataType, chain_link<DataType>, _chain_end<DataType>, LinkVal>
{
};

/// Bidirectional and single ended chain
template<class DataType, bichain_link<DataType> DataType::* LinkVal>
class bichain : public _chain_impl
    <DataType, bichain_link<DataType>, _chain_end<DataType>, LinkVal>
{
};

/// Directional and double ended chain
template<class DataType, chain_link<DataType> DataType::* LinkVal>
class dechain : public _chain_impl
    <DataType, chain_link<DataType>, _dechain_end<DataType>, LinkVal>
{
};

/// Bidirectional and double ended chain
template<class DataType, bichain_link<DataType> DataType::* LinkVal>
class bidechain : public _chain_impl
    <DataType, bichain_link<DataType>, _dechain_end<DataType>, LinkVal>
{
};


#endif  // Include guards.
