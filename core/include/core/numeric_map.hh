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

#ifndef CORE_NUMERIC_MAP_HH_
#define CORE_NUMERIC_MAP_HH_

#include <core/mempool.hh>


/// @brief 数値をキーとするマップクラス
/// @tparam KEY_TYPE  キーのデータ型。整数型でなければならない。
/// @tparam VAL_TYPE  格納する値のデータ型。実際に扱う型は VAL_TYPE* になる。
/// @tparam KEY       キーの値を返す VAL_TYPE のメンバ関数。
///                   キーと値は１対１で対応しなければならず、そのキーは
///                   値のメンバ関数 KEY を介して参照できなければならない。
/// @tparam CHAIN_NODE  VAL_TYPE は numeric_map による管理のために bichain_node
///                     をメンバ変数として持つ必要がある。そのメンバ変数を参照
///                     する VAL_TYPE のメンバ関数。
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

	auto mp = mempool::acquire_shared(sizeof (dict_ent) * dict_cnt);
	if (is_fail(mp))
		return mp.cause();

	dict_mp = mp;
	dict = new (*dict_mp) dict_ent[dict_cnt];

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

	operator delete (dict, *dict_mp);

	return mempool::release_shared(dict_mp);
}


#endif  // include guard

