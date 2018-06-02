/// @file  util/chain.hh
/// @brief Link list structure.

//  Uniqos  --  Unique Operating System
//  (C) 2010 KATO Takeshi
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
template<class OBJ_T>
class forward_chain_node
{
    template<class, class, class, class>
    friend class chain_impl_;

    OBJ_T* next;

    const forward_chain_node<OBJ_T>& operator() () const { return *this; }
          forward_chain_node<OBJ_T>& operator() ()       { return *this; }

    void _set_prev(OBJ_T*) { /* nothing */ }
    void _set_next(OBJ_T* p) { next = p; }

public:
    const OBJ_T* get_next() const { return next; }
          OBJ_T* get_next()       { return next; }
};

/// @brief  Bidirectional chain node.
template<class OBJ_T>
class chain_node : public forward_chain_node<OBJ_T>
{
    template<class, class, class, class>
    friend class chain_impl_;

    OBJ_T* prev;

    const chain_node<OBJ_T>& operator() () const { return *this; }
          chain_node<OBJ_T>& operator() ()       { return *this; }

    void _set_prev(OBJ_T* p) { prev = p; }

public:
    const OBJ_T* get_prev() const { return prev; }
          OBJ_T* get_prev()       { return prev; }
};

/// @brief  Single side chain.
template<class OBJ_T>
class front_chain_end_
{
    OBJ_T* front;

public:
    front_chain_end_() :
        front(0)
    {}

    void set_front(OBJ_T* p) { front = p; }
    void set_back(OBJ_T*) { /* nothing */ }
    const OBJ_T* get_front() const { return front; }
          OBJ_T* get_front()       { return front; }
};

/// @brief  Double side chain.
template<class OBJ_T>
class back_chain_end_ : public front_chain_end_<OBJ_T>
{
    OBJ_T* back;

public:
    back_chain_end_() :
        front_chain_end_<OBJ_T>(),
        back(0)
    {}

    void set_back(OBJ_T* p) { back = p; }
    const OBJ_T* get_back() const { return back; }
          OBJ_T* get_back()       { return back; }
};

/// @brief  Chain implemnts.
template<
    class OBJ_T,
    class NODE_T,
    class END,
    class NODEOF_T>
class chain_impl_ : NODEOF_T
{
    typedef chain_impl_<OBJ_T, NODE_T, END, NODEOF_T> self_t;

    const NODE_T& nodeof(const OBJ_T* obj) const {
        return NODEOF_T::operator() (obj);
    }
    NODE_T& nodeof(OBJ_T* obj) {
        return NODEOF_T::operator() (obj);
    }
    void set_prev(OBJ_T* p, OBJ_T* prev) {
        nodeof(p)._set_prev(prev);
    }
    void set_next(OBJ_T* p, OBJ_T* next) {
        nodeof(p)._set_next(next);
    }

protected:
    END chain_end;

public:
    using obj_t = OBJ_T;

    class iterator : NODEOF_T
    {
        OBJ_T* data;
        NODE_T& node() {
            return NODEOF_T::operator() (data);
        }

    public:
        iterator(const NODEOF_T nodeof, OBJ_T* _data) :
            NODEOF_T(nodeof), data(_data)
        {}
        iterator(const iterator& it) :
            NODEOF_T(it), data(it.data)
        {}
        ~iterator() {}

    public:
        OBJ_T* operator* () {
            return data;
        }
        OBJ_T* operator-> () {
            return data;
        }
        iterator& operator= (iterator& it) {
            data = it.data;
            return *this;
        }
        bool operator== (const iterator& other) const {
            return data == other.data;
        }
        bool operator!= (const iterator& other) const {
            return data != other.data;
        }
        iterator& operator++ () {
            data = node().get_next();
            return *this;
        }
        iterator operator++ (int) {
            iterator tmp(data);
            data = node().get_next();
            return tmp;
        }
        iterator& operator-- () {
            data = node().get_prev();
            return *this;
        }
        iterator operator-- (int) {
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
        OBJ_T* ref() {
            return data;
        }
    };

    class const_iterator : NODEOF_T
    {
        const OBJ_T* data;
        const NODE_T& node() {
            return NODEOF_T::operator() (data);
        }

    public:
        const_iterator(const NODEOF_T& nodeof, const OBJ_T* _data) :
            NODEOF_T(nodeof), data(_data)
        {}
        const_iterator(const const_iterator& it) :
            NODEOF_T(it), data(it.data)
        {}
        ~const_iterator() {}

    public:
        const OBJ_T* operator* () const {
            return data;
        }
        const OBJ_T* operator-> () const {
            return data;
        }
        const_iterator& operator= (const_iterator& it) {
            data = it.data;
            return *this;
        }
        bool operator== (const const_iterator& other) const {
            return data == other.data;
        }
        bool operator!= (const const_iterator& other) const {
            return data != other.data;
        }
        const_iterator& operator++ () {
            data = node().get_next();
            return *this;
        }
        const_iterator operator++ (int) {
            const_iterator tmp(data);
            data = node().get_next();
            return tmp;
        }
        const_iterator& operator-- () {
            data = node().get_prev();
            return *this;
        }
        const_iterator operator-- (int) {
            const_iterator tmp(data);
            data = node().get_prev();
            return tmp;
        }

    public:
        const_iterator next() const {
            return const_iterator(*this, data->get_next());
        }
        const_iterator prev() const {
            return const_iterator(*this, data->get_prev());
        }

    public:
        const OBJ_T* ref() const {
            return data;
        }
    };

    chain_impl_(const NODEOF_T& nodeof = NODEOF_T()) :
        NODEOF_T(nodeof), chain_end()
    {}

    iterator begin() {
        return iterator(*this, chain_end.get_front());
    }
    const_iterator begin() const {
        return const_iterator(*this, chain_end.get_front());
    }

    iterator end() {
        return iterator(*this, nullptr);
    }
    const_iterator end() const {
        return const_iterator(*this, nullptr);
    }

    // any
    const OBJ_T* front() const { return chain_end.get_front(); }
          OBJ_T* front()       { return chain_end.get_front(); }

    // !front_chain !front_forward_chain
    const OBJ_T* back() const { return chain_end.get_back(); }
          OBJ_T* back()       { return chain_end.get_back(); }

    bool is_empty() const { return chain_end.get_front() == 0; }

    void move_to(self_t* to) {
        to->chain_end.set_front(chain_end.get_front());
        to->chain_end.set_back(chain_end.get_back());
        chain_end.set_front(nullptr);
        chain_end.set_back(nullptr);
    }

    // !forward_chain !front_froward_chain
    const OBJ_T* prev(const OBJ_T* p) const {
        return nodeof(p).prev;
    }
          OBJ_T* prev(OBJ_T* p) {
        return nodeof(p).prev;
    }

    // 1end, 2end & dir, bidir
    const OBJ_T* next(const OBJ_T* p) const {
        return nodeof(p).next;
    }
          OBJ_T* next(OBJ_T* p) {
        return nodeof(p).next;
    }

    // any
    void push_front(OBJ_T* p) {
        set_prev(p, 0);
        set_next(p, front());
        if (front() != nullptr)
            set_prev(front(), p);
        else
            chain_end.set_back(p);
        chain_end.set_front(p);
    }
    // !front_chain !front_forward_chain
    void push_back(OBJ_T* p) {
        set_prev(p, back());
        set_next(p, 0);
        if (front() != nullptr)
            set_next(back(), p);
        else
            chain_end.set_front(p);
        chain_end.set_back(p);
    }
    // !forward_chain !front_forward_chain
    void insert_before(OBJ_T* base, OBJ_T* p) {
        set_prev(p, prev(base));
        set_next(p, base);
        if (prev(base))
            set_next(prev(base), p);
        else
            chain_end.set_front(p);
        set_prev(base, p);
    }
    // any
    void insert_after(OBJ_T* base, OBJ_T* p) {
        set_prev(p, base);
        set_next(p, next(base));
        if (next(base))
            set_prev(next(base), p);
        else
            chain_end.set_back(p);
        set_next(base, p);
    }
    // any
    OBJ_T* pop_front() {
        OBJ_T* h = front();
        if (h) {
            OBJ_T* h2 = next(h);
            chain_end.set_front(h2);
            if (h2 != nullptr)
                set_prev(h2, 0);
            else
                chain_end.set_back(0);
        }
        return h;
    }
    // chain only
    OBJ_T* pop_back() {
        OBJ_T* t = back();
        if (t) {
            OBJ_T* t2 = prev(t);
            chain_end.set_back(t2);
            if (t2 != nullptr)
                set_next(t2, 0);
            else
                chain_end.set_front(0);
        }
        return t;
    }
    // 1end, 2end & bidir
    void remove(OBJ_T* p) {
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
    void remove_next(OBJ_T* p) {
        OBJ_T* next1 = next(p);
        if (next1) {
            OBJ_T* next2 = next(next1);
            set_next(p, next2);
            if (next2)
                set_prev(next2, p);
            else
                chain_end.set_back(p);
        }
    }
    // 1end, 2end & dir, bidir
    // low performance for long chain.
    void find_and_remove(OBJ_T* p) {
        if (front() == p) {
            pop_front();
        } else {
            for (OBJ_T* t = front(); t; t = next(t)) {
                if (next(t) == p)
                    remove_next(t);
            }
        }
    }
};

template<class CHAIN, class FILTER>
class chain_filter
{
    using obj_t = typename CHAIN::obj_t;

    CHAIN* _chain;
    FILTER _filter;

public:
    class iter
    {
        CHAIN& chain;
        chain_filter<CHAIN, FILTER>* _cf;
        obj_t* _value;
    public:
        iter(CHAIN& _chain, chain_filter<CHAIN, FILTER>* cf, obj_t* value) :
            chain(_chain),
            _cf(cf),
            _value(value)
        {}
        ~iter() {}

    public:
        obj_t* operator* () {
            return _value;
        }
        obj_t* operator-> () {
            return _value;
        }
        bool operator== (const iter& other) const {
            return _value == other._value;
        }
        bool operator!= (const iter& other) const {
            return _value != other._value;
        }
        iter& operator++ () {
            _value = _cf->find_next(chain.next(_value));
            return *this;
        }
        iter operator++ (int) {
            iter copy(_cf, _value);
            _value = _cf->find_next(chain.next(_value));
            return copy;
        }
        iter& operator-- () {
            _value = _cf->find_prev(chain.prev(_value));
            return *this;
        }
        iter operator-- (int) {
            iter copy(_cf, _value);
            _value = _cf->find_prev(chain.prev(_value));
            return copy;
        }
    };

public:
    chain_filter(const chain_filter& other) :
        _chain(other._chain),
        _filter(other._filter)
    {}
    chain_filter(CHAIN* chain, FILTER&& filter) :
        _chain(chain),
        _filter(filter)
    {}
    ~chain_filter() {}

public:
    iter begin() {
        return iter(*_chain, this, find_next(_chain->front()));
    }
    iter end() {
        return iter(*_chain, this, nullptr);
    }
    iter rbegin() {
        return iter(*_chain, this, find_next(_chain->back()));
    }
    iter rend() {
        return iter(*_chain, this, nullptr);
    }

private:
    obj_t* find_next(obj_t* next) {
        while (next && !_filter(next))
            next = _chain->next(next);
        return next;
    }
    obj_t* find_prev(obj_t* prev) {
        while (prev && !_filter(prev))
            prev = _chain->prev(prev);
        return prev;
    }
};

template <class OBJ_T, class NODE_T, NODE_T OBJ_T::* NODE>
class chain_node_of_
{
public:
    const NODE_T& operator() (const OBJ_T* obj) const {
        return obj->*NODE;
    }
    NODE_T& operator() (OBJ_T* obj) {
        return obj->*NODE;
    }
};

/// Directional one side chain
// flexible
template<class OBJ_T, class NODEOF_T>
using front_forward_fchain = chain_impl_<
    OBJ_T,
    forward_chain_node<OBJ_T>,
    front_chain_end_<OBJ_T>,
    NODEOF_T>;

// unflexible
template<class OBJ_T, forward_chain_node<OBJ_T> OBJ_T::* MEMBER>
using front_forward_chain = chain_impl_<
    OBJ_T,
    forward_chain_node<OBJ_T>,
    front_chain_end_<OBJ_T>,
    chain_node_of_<OBJ_T, forward_chain_node<OBJ_T>, MEMBER>>;


/// Bidirectional one side chain
// flexible
template<class OBJ_T, class NODEOF_T>
using front_fchain = chain_impl_<
    OBJ_T,
    chain_node<OBJ_T>,
    front_chain_end_<OBJ_T>,
    NODEOF_T>;

// unflexible
template<class OBJ_T, chain_node<OBJ_T> OBJ_T::* MEMBER>
using front_chain = chain_impl_<
    OBJ_T,
    chain_node<OBJ_T>,
    front_chain_end_<OBJ_T>,
    chain_node_of_<OBJ_T, chain_node<OBJ_T>, MEMBER>>;


/// Directional both side chain
// flexible
template<class OBJ_T, class NODEOF_T>
using forward_fchain = chain_impl_<
    OBJ_T,
    forward_chain_node<OBJ_T>,
    back_chain_end_<OBJ_T>,
    NODEOF_T>;

// unflexible
template<class OBJ_T, forward_chain_node<OBJ_T> OBJ_T::* MEMBER>
using forward_chain = chain_impl_<
    OBJ_T,
    forward_chain_node<OBJ_T>,
    back_chain_end_<OBJ_T>,
    chain_node_of_<OBJ_T, forward_chain_node<OBJ_T>, MEMBER>>;


/// Bidirectional both side chain
// flexible
template<class OBJ_T, class NODEOF_T>
using fchain = chain_impl_<
    OBJ_T,
    chain_node<OBJ_T>,
    back_chain_end_<OBJ_T>,
    NODEOF_T>;

// unflexible
template<class OBJ_T, chain_node<OBJ_T> OBJ_T::* MEMBER>
using chain = chain_impl_<
    OBJ_T,
    chain_node<OBJ_T>,
    back_chain_end_<OBJ_T>,
    chain_node_of_<OBJ_T, chain_node<OBJ_T>, MEMBER>>;


#endif  // UTIL_CHAIN_HH_

