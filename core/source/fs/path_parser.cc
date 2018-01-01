/// @file  path_parser.cc
/// @brief Filepath operations.

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

#include <core/fs_ctl.hh>

#include <core/process.hh>
#include <util/string.hh>


namespace {

void cnt_nodes_and_len(const char* path, int* nodes, int* len)
{
	int _nodes = 0;
	int _len = 0;

	for (;;) {
		path = fs::path_skip_splitter(path);

		if (*path == '\0')
			break;

		if (fs::path_skip_current(&path)) {
			continue;
		} else if (fs::path_skip_parent(&path)) {
			continue;
		}

		uptr namelen = fs::name_length(path);
		path += namelen;
		_len += namelen + 1;

		_nodes += 1;
	}

	*nodes = _nodes;
	*len = _len;
}

}  // namespace

namespace fs {

/// @class path_parser
/// path_parser::create() へパス名を渡すと、解析結果が path_parser のインス
/// タンスとして返される。
/// パス名の最後に存在しないノードがある場合は、get_edge_node()->fsnode が
/// nullptr になる。
/// 存在しないノードの先を参照しようとすると（それが親ノードであっても）
/// cause::NOENT を返す。
/// ノードを作成しようとしているときに、get_edge_node()->fsnode が nullptr
/// であれば、まだ同名のノードが無いと判断できる。
/// 既存のノードを参照するときは path_parser::create() の戻り値が cause::OK、
/// かつ、get_edge_node()->fsnode != nullptr であることを確認する必要がある。

path_parser::path_parser(generic_ns* fsns, node_off node_nr) :
	ns(fsns),
	node_buf_nr(node_nr),
	node_use_nr(0),
	path_end(get_path_buffer())
{
	fs_node* root = static_cast<fs_ns*>(ns)->ref_root();
	nodes[0].fsnode = root;
	nodes[0].name = path_end;
	*path_end++ = SPLITTER;
	node_use_nr += 1;
}

path_parser::~path_parser()
{
	for (node_off i = 0; i < node_use_nr; ++i) {
		fs_node* fsn = nodes[i].fsnode;
		if (fsn)
			fsn->refs.dec();
	}
}

cause::pair<path_parser*> path_parser::_create(
    generic_ns* ns, u16 node_nr, u16 path_len)
{
	uptr bytes = sizeof (path_parser) +
	             sizeof (node) * node_nr +
	             path_len + 1;

	path_parser* obj =
	    new (generic_mem().allocate(bytes)) path_parser(ns, node_nr);
	if (!obj)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, obj);
}

cause::pair<path_parser*> path_parser::_create(
    generic_ns* ns,
    const char* cwd,
    const char* path)
{
	node_off node_cnt = 1;  // for root node.
	u16 path_len = 0;

	int _nodes, _len;
	if (path_is_relative(path)) {
		// count cwd.
		cnt_nodes_and_len(cwd, &_nodes, &_len);
		node_cnt += _nodes;
		path_len += _len;
	}

	// count path.
	cnt_nodes_and_len(path, &_nodes, &_len);
	node_cnt += _nodes;
	path_len += _len;

	auto _obj = _create(ns, node_cnt, path_len);
	if (is_fail(_obj))
		return null_pair(cause::NOMEM);

	path_parser* obj = _obj.value();

	if (path_is_relative(path)) {
		// follow cwd.
		cause::t r = obj->follow_path(cwd);
		if (is_fail(r)) {
			destroy(obj);
			return null_pair(r);
		}
	}

	// follow path.
	cause::t r = obj->follow_path(path);
	if (r == cause::NOENT) {
		// do nothing. same as cause::OK.
	} if (is_fail(r)) {
		destroy(obj);
		return null_pair(r);
	}

	*obj->path_end = '\0';

	return make_pair(r, obj);
}

cause::pair<path_parser*> path_parser::optimize()
{
	auto r = _create(ns, node_use_nr, path_end - get_path_buffer());
	if (is_fail(r))
		return r;

	path_parser* y = r.value();

	char* my_path = get_path_buffer();
	char* your_path = y->get_path_buffer();
	str_copy(my_path, your_path);

	for (node_off i = 0; i < node_use_nr; ++i) {
		y->nodes[i].fsnode = nodes[i].fsnode;

		uptr path_off = nodes[i].name - my_path;
		y->nodes[i].name = your_path + path_off;
	}

	y->node_use_nr = node_use_nr;
	node_use_nr = 0;

	uptr path_off = path_end - my_path;
	y->path_end = your_path + path_off;

	return make_pair(cause::OK, y);
}

cause::pair<path_parser*> path_parser::create(
    generic_ns* ns,
    const char* cwd,
    const char* path)
{
	auto r = _create(ns, cwd, path);
	if (is_fail(r))
		return r;

	path_parser* x = r.value();

	sptr len = str_length(cwd) + str_length(path);

	// 確保したバッファの半分しか使っていなければ、
	// 余ったバッファを開放して作り直す。
	if (x->node_use_nr < x->node_buf_nr / 2 ||
	    x->path_end - x->get_path_buffer() < len / 2)
	{
		auto r2 = x->optimize();
		if (is_fail(r2)) {
			if (r2.value())
				destroy(r2.value());
			return r;
		}
		r = r2;
		destroy(x);
	}

	return r;
}

cause::pair<path_parser*> path_parser::create(
    process* proc,
    const char* path)
{
	spin_rlock_section ns_lock_sec(proc->ref_ns_lock());
	spin_rlock_section cwd_lock_sec(proc->ref_cwd_lock());

	generic_ns* proc_ns = proc->get_ns_fs();
	const char* proc_cwd = proc->get_cwd_path();

	return path_parser::create(proc_ns, proc_cwd, path);
}

void path_parser::destroy(path_parser* x)
{
	x->~path_parser();
	operator delete (x, generic_mem());
}

const char* path_parser::get_path() const
{
	return const_cast<path_parser*>(this)->get_path_buffer();
}

const path_parser::node* path_parser::get_node(node_off i)
{
	return i < node_use_nr ? &nodes[i] : nullptr;
}

fs_node* path_parser::get_fsnode(node_off i)
{
	return i < node_use_nr ? nodes[i].fsnode : nullptr;
}

const char* path_parser::get_name(node_off i)
{
	return i < node_use_nr ? nodes[i].name : nullptr;
}

/// Same as path_parser::get_node(path_parser::get_node_nr() - 1)
const path_parser::node* path_parser::get_edge_node()
{
	return edge_node();
}

/// Same as path_parser::get_fsnode(path_parser::get_node_nr() - 1)
fs_node* path_parser::get_edge_fsnode()
{
	return edge_node()->fsnode;
}

/// Same as path_parser::get_name(path_parser::get_node_nr() - 1)
const char* path_parser::get_edge_name()
{
	return edge_node()->name;
}

/// @param path  Relative path. Head splitter will be ignored.
cause::t path_parser::follow_path(const char* path)
{
	fs_node* fsnode = edge_node()->fsnode;

	for (;;) {
		path = path_skip_splitter(path);

		if (*path == '\0')
			break;

		if (!fsnode)
			return cause::NOENT;

		if (path_skip_current(&path)) {
			continue;
		} else if (path_skip_parent(&path)) {
			if (!pop_node())
				return cause::NOENT;
			fsnode = edge_node()->fsnode;
			continue;
		}

		auto child = fsnode->ref_child_node(path);
		if (child.cause() == cause::NOENT) {
			fsnode = nullptr;
		} if (is_fail(child)) {
			return child.cause();
		} else {
			fsnode = child.value()->ref_into_ns(ns);
		}
		push_node(fsnode, path);
	}

	return cause::OK;
}

char* path_parser::get_path_buffer()
{
	return reinterpret_cast<char*>(&nodes[node_buf_nr]);
}

void path_parser::push_node(fs_node* fsnode, const char* name)
{
	nodes[node_use_nr].fsnode = fsnode;

	*path_end++ = SPLITTER;

	nodes[node_use_nr].name = path_end;

	path_end += name_normalize(name, path_end);

	++node_use_nr;
}

bool path_parser::pop_node()
{
	// Even if try to find parent of root node, it remains root node.
	// So "/.." is same as "/".
	if (node_use_nr <= 1) {
		// If you change this return value to false, make error
		// when you try to find parent of root node.
		return true;
	}

	--node_use_nr;

	nodes[node_use_nr].fsnode->refs.dec();

	path_end = const_cast<char*>(
	    path_rskip_splitter(nodes[node_use_nr].name));

	return true;
}

path_parser::node* path_parser::edge_node()
{
	return &nodes[node_use_nr - 1];
}

}  // namespace fs

