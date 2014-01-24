/// @file   core/include/core/numeric_map.hh
/// @brief  numeric_map class declaration and definition.

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CORE_INCLUDE_CORE_NUMERIC_MAP_HH_
#define CORE_INCLUDE_CORE_NUMERIC_MAP_HH_

#include <basic.hh>
#include <chain.hh>
#include <mempool.hh>


template<
    class KEY_TYPE,
    class VAL_TYPE,
    KEY_TYPE (VAL_TYPE::* KEY)() const,
    bichain_node<VAL_TYPE>& (VAL_TYPE::* CHAIN_NODE)()
>
class numeric_map
{
	typedef bichain<VAL_TYPE, CHAIN_NODE> dict_ent;

public:
	cause::t init(int dict_size_shifts);
	cause::t uninit();

	cause::pair<VAL_TYPE*> at(const KEY_TYPE& key) {
		dict_ent* d = dict[key & dict_mask];
		for (VAL_TYPE* val = d->front(); val; val = d->next(val)) {
			if (val->*KEY() == key)
				return cause::pair<VAL_TYPE*>(cause::OK, val);
		}
		return cause::pair<VAL_TYPE*>(cause::NOT_FOUND, 0);
	}

	cause::t insert(VAL_TYPE* val) {
		dict_ent* d = dict[val->*KEY() & dict_mask];
		d->insert_head(val);
		return cause::OK;
	}

	cause::t erase(VAL_TYPE* val) {
		dict_ent* d = dict[val->*KEY() & dict_mask];
		d->remove(val);
		return cause::OK;
	}

private:
	dict_ent* dict;
	KEY_TYPE dict_mask;
	KEY_TYPE dict_cnt;
	mempool* dict_mp;
};

template<
    class KEY_TYPE,
    class VAL_TYPE,
    KEY_TYPE (VAL_TYPE::* KEY)() const,
    bichain_node<VAL_TYPE>& (VAL_TYPE::* CHAIN_NODE)()
>
cause::t numeric_map<KEY_TYPE, VAL_TYPE, KEY, CHAIN_NODE>::init(
    int dict_size_shifts)
{
	dict_cnt = 1 << dict_size_shifts;
	dict_mask = dict_cnt - 1;

	cause::t r = mempool_acquire_shared(
	    sizeof (dict_ent) * dict_cnt, &dict_mp);
	if (is_fail(r))
		return r;

	dict = new (dict_mp) dict_ent[dict_cnt];

	return cause::OK;
}

template<
    class KEY_TYPE,
    class VAL_TYPE,
    KEY_TYPE (VAL_TYPE::* KEY)() const,
    bichain_node<VAL_TYPE>& (VAL_TYPE::* CHAIN_NODE)()
>
cause::t numeric_map<KEY_TYPE, VAL_TYPE, KEY, CHAIN_NODE>::uninit()
{
	for (KEY_TYPE i = 0; i < dict_cnt; ++i)
		dict[i].~dict_ent();

	operator delete (dict, dict_mp);

	return mempool_release_shared(dict_mp);
}


#endif  // include guard
