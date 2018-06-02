/// @file  util/foreach.hh
/// @brief foreach interface.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
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

#ifndef UTIL_FOREACH_HH_
#define UTIL_FOREACH_HH_


template <class DATA, class ITERATOR = DATA>
class foreach
{
public:
    struct interfaces
    {
        void init() {
            Next    = nullptr;
            GetData = nullptr;
            IsEqual = nullptr;
            Dtor    = nullptr;
        }

        using NextIF = ITERATOR (*)(void* context, ITERATOR current);
        NextIF Next;

        using GetDataIF = DATA (*)(void* context, ITERATOR current);
        GetDataIF GetData;

        using IsEqualIF = bool (*)(const void* context,
            ITERATOR, ITERATOR);
        IsEqualIF IsEqual;

        using DtorIF = void (*)(void* context);
        DtorIF Dtor;
    };

    class iterator
    {
    public:
        iterator(foreach<DATA, ITERATOR>& owner, ITERATOR itr) :
            _owner(owner), _itr(itr)
        {}
        ~iterator()
        {}

        DATA operator * () {
            return _owner.getdata(_itr);
        }
        void operator ++ () {
            _itr = _owner.next(_itr);
        }
        bool operator != (const iterator& y) const {
            return !_owner.isequal(_itr, y._itr);
        }

    private:
        foreach<DATA, ITERATOR>& _owner;
        ITERATOR _itr;
    };

public:
    foreach(interfaces* ifs, void* context, ITERATOR begin, ITERATOR end) :
        _ifs(ifs),
        _context(context),
        _begin(begin),
        _end(end)
    {}

    foreach(foreach<DATA, ITERATOR>&& src) :
        _ifs(src._ifs),
        _context(src._context),
        _begin(src._begin),
        _end(src._end)
    {
        src._ifs = nullptr;
        src._context = nullptr;
    }

    ~foreach() {
        if (_ifs)
            _ifs->Dtor(_context);
    }

    iterator begin() {
        return iterator(*this, _begin);
    }
    iterator end() {
        return iterator(*this, _end);
    }

    ITERATOR next(ITERATOR itr) {
        return _ifs->Next(_context, itr);
    }
    DATA getdata(ITERATOR itr) {
        return _ifs->GetData(_context, itr);
    }
    bool isequal(const ITERATOR itr1, const ITERATOR itr2) const {
        return _ifs->IsEqual(_context, itr1, itr2);
    }

private:
    interfaces* _ifs;
    void* _context;
    ITERATOR _begin;
    ITERATOR _end;
};


template <class CHAIN, class EXPORT_TYPE=typename CHAIN::obj_t>
class locked_chain_iterator
{
    class iterator;
    using obj_t = typename CHAIN::obj_t;

public:
    locked_chain_iterator(CHAIN& _chain, obj_t* _start, spin_rwlock* _lock) :
        chain(_chain),
        start(_start),
        lock(_lock)
    {
        if (lock)
            lock->rlock();
    }
    locked_chain_iterator(locked_chain_iterator&& src) :
        chain(src.chain),
        start(src.start),
        lock(src.lock)
    {
        src.lock = nullptr;
    }

    ~locked_chain_iterator()
    {
        if (lock)
            lock->un_rlock();
    }

    iterator begin() {
        return iterator(chain, start);
    }
    iterator end() {
        return iterator(chain, nullptr);
    }

private:
    CHAIN& chain;
    obj_t* start;
    spin_rwlock* lock;
};

template <class CHAIN, class EXPORT_TYPE>
class locked_chain_iterator<CHAIN, EXPORT_TYPE>::iterator
{
    using chain_t = CHAIN;
    using export_t = EXPORT_TYPE;

public:
    iterator(CHAIN& _chain, obj_t* start) :
        chain(_chain),
        current(start)
    {}
    iterator(iterator&& src) : 
        chain(src.chain),
        current(src.current)
    {}

    export_t* operator * () {
        return static_cast<export_t*>(current);
    }
    export_t* operator ++ () {
        current = chain.next(current);
        return static_cast<export_t*>(current);
    }
    bool operator == (const iterator& itr) const {
        return current == itr.current;
    }
    bool operator != (const iterator& itr) const {
        return current != itr.current;
    }

private:
    CHAIN& chain;
    obj_t* current;
};


#endif  // UTIL_FOREACH_HH_

