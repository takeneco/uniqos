/// @file   core/devnode.cc
/// @brief  devnode implementation.

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

#include <core/devnode.hh>

#include <arch.hh>
#include <core/global_vars.hh>
#include <core/new_ops.hh>
#include <core/setup.hh>


devnode_no make_dev(int major, int minor)
{
	return devnode_no(major & 0xffff) << 16 | devnode_no(minor & 0xffff);
}

int get_dev_major(devnode_no dev)
{
	return (dev >> 16) & 0xffff;
}

int get_dev_minor(devnode_no dev)
{
	return dev & 0xffff;
}

/// @class dev_node_ctl
/// @brief
/// device numberを管理する。

dev_node_ctl::dev_node_ctl() :
	minor_set_mp(nullptr),
	major_set_mp(nullptr)
{
}

dev_node_ctl::~dev_node_ctl()
{
}

cause::t dev_node_ctl::setup()
{
	auto mp = mempool::acquire_shared(sizeof (minor_set));
	if (is_fail(mp))
		return mp.cause();

	minor_set_mp = mp.data();

	mp = mempool::acquire_shared(sizeof (major_set));
	if (is_fail(mp))
		return mp.cause();

	major_set_mp = mp.data();

	return cause::OK;
}

cause::pair<io_node*> dev_node_ctl::search(int maj, int min)
{
	auto major = search_major(maj);
	if (is_fail(major))
		return null_pair(cause::NODEV);

	for (auto set : major.data()->minor_chain) {
		if (set->minor == min)
			return make_pair(cause::OK, set->node);
	}

	return null_pair(cause::NODEV);
}

cause::pair<int> dev_node_ctl::assign_major(int maj_from, int maj_to)
{
	int maj;
	for (maj = maj_from; maj <= maj_to; ++maj) {
		auto r = search_major(maj);
		if (r.cause() == cause::NODEV)
			break;
	}

	if (maj > maj_to)
		return zero_pair(cause::FAIL);

	major_set* set = new (*major_set_mp) major_set(maj);
	if (!set)
		return zero_pair(cause::NOMEM);

	major_set_pool.push_front(set);

	return make_pair(cause::OK, maj);
}

cause::pair<int> dev_node_ctl::assign_minor(
    io_node* ion,
    int maj,
    int min_from,
    int min_to)
{
	auto major = search_major(maj);
	if (is_fail(major))
		return zero_pair(major.cause());

	int min;
	for (min = min_from; min <= min_to; ++min) {
		auto r = search_minor(major.data(), min);
		if (r.cause() == cause::NODEV)
			break;
	}

	if (min > min_to)
		return zero_pair(cause::FAIL);

	minor_set* min_set = new (*minor_set_mp) minor_set(min, ion);
	if (!min_set)
		return zero_pair(cause::NOMEM);

	major.data()->minor_chain.push_front(min_set);

	return make_pair(cause::OK, min);
}

cause::pair<dev_node_ctl::major_set*> dev_node_ctl::search_major(int maj)
{
	for (auto maj_set : major_set_pool) {
		if (maj == maj_set->major)
			return make_pair(cause::OK, maj_set);
	}

	return null_pair(cause::NODEV);
}

cause::pair<dev_node_ctl::minor_set*> dev_node_ctl::search_minor(
    major_set* major, int min)
{
	for (auto min_set : major->minor_chain) {
		if (min == min_set->minor)
			return make_pair(cause::OK, min_set);
	}

	return null_pair(cause::NODEV);
}


cause::pair<io_node*> devnode_search(int maj, int min)
{
	dev_node_ctl* ctl = global_vars::core.dev_node_ctl_obj = ctl;

	return ctl->search(maj, min);
}

cause::pair<devnode_no> devnode_assign_minor(
    io_node* ion,
    int maj,
    int min_from,
    int min_to)
{
	dev_node_ctl* ctl = global_vars::core.dev_node_ctl_obj = ctl;

	ctl->assign_minor(ion, maj, min_from, min_to);
}

cause::t devnode_setup()
{
	dev_node_ctl* ctl = new (generic_mem()) dev_node_ctl;
	if (!ctl)
		return cause::NOMEM;

	auto r = ctl->setup();
	if (is_fail(r))
		return r;

	global_vars::core.dev_node_ctl_obj = ctl;

	return cause::OK;
}

