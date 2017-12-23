/// @file  ns.cc
/// @brief Namespace control.

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

#include <core/ns.hh>

#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>


class ns_ctl;

// local functions

namespace {

ns_ctl* get_ns_ctl()
{
	return global_vars::core.ns_ctl_obj;
}

}  // namespace


/// @brief  Namespace controller.
class ns_ctl
{
public:
	ns_ctl();

public:
	cause::t setup();
	cause::t teardown();

	cause::pair<generic_ns*> create_ns(generic_ns::TYPE t);
	void inc_ref(generic_ns* ns);
	void dec_ref(generic_ns* ns);

private:
	chain<generic_ns, &generic_ns::ns_ctl_chainnode> ns_chain;

	mempool* generic_ns_mp;
};

ns_ctl::ns_ctl() :
	generic_ns_mp(nullptr)
{
}

cause::t ns_ctl::setup()
{
	auto mp = mempool::acquire_shared(sizeof (generic_ns));
	if (is_fail(mp))
		return mp.cause();

	generic_ns_mp = mp.value();

	return cause::OK;
}

cause::t ns_ctl::teardown()
{
	if (generic_ns_mp) {
		mempool::release_shared(generic_ns_mp);
		generic_ns_mp = nullptr;
	}

	uint ns_cnt = 0;
	for (auto x : ns_chain)
		++ns_cnt;
	log()(SRCPOS)("warn: fs_ns exists: ").u(ns_cnt)();

	return cause::OK;
}

cause::pair<generic_ns*> ns_ctl::create_ns(generic_ns::TYPE t)
{
	generic_ns* ns = new (*generic_ns_mp) generic_ns(t);
	if (!ns)
		return null_pair(cause::NOMEM);

	ns_chain.push_front(ns);

	return make_pair(cause::OK, ns);
}

void ns_ctl::inc_ref(generic_ns* ns)
{
	ns->refs.inc();
}

void ns_ctl::dec_ref(generic_ns* ns)
{
	ns->refs.dec();

	// TODO: 0 になった直後に inc_ref() された場合に備えて、遅延開放する。
	if (ns->refs.load() == 0) {
		ns_chain.remove(ns);
		new_destroy(ns, generic_mem());
	}
}

// generic_ns

generic_ns::generic_ns(TYPE t) :
	refs(0),
	type(t)
{
}

// ns_set

ns_set::ns_set() :
	ns_fs(nullptr)
{
}

// external interfaces

cause::t ns_setup()
{
	ns_ctl* _ns_ctl = new (generic_mem()) ns_ctl();
	if (!_ns_ctl)
		return cause::NOMEM;

	cause::t r = _ns_ctl->setup();
	if (is_fail(r))
		return r;

	global_vars::core.ns_ctl_obj = _ns_ctl;

	return cause::OK;
}

cause::t ns_teardown()
{
	ns_ctl* _ns_ctl = global_vars::core.ns_ctl_obj;
	cause::t r = _ns_ctl->teardown();
	if (is_fail(r))
		return r;

	r = new_destroy(_ns_ctl, generic_mem());
	if (is_fail(r))
		return r;

	global_vars::core.ns_ctl_obj = nullptr;

	return cause::OK;
}

/// @brief  Create new filesystem namespace.
//
/// Initial reference count is 1 of new namespace.
cause::pair<generic_ns*> create_fs_ns()
{
	return get_ns_ctl()->create_ns(generic_ns::TYPE_FS);
}

/// @brief Increment reference count of namespace.
void ns_inc_ref(generic_ns* ns)
{
	get_ns_ctl()->inc_ref(ns);
}

/// @brief Decrement reference count of namespace.
//
/// This function release namespace if reference count become 0.
void ns_dec_ref(generic_ns* ns)
{
	get_ns_ctl()->dec_ref(ns);
}

