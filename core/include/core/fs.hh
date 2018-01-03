/// @file  core/fs.hh
/// @brief  Filesystem interfaces.

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

#ifndef CORE_FS_HH_
#define CORE_FS_HH_

#include <core/ns.hh>


class process;

class fs_node;

namespace fs {

typedef u16 pathlen_t;  ///< pathname char count type.

const char SPLITTER = '/';
const char ESCAPE = '\\';

const char CURRENT[] = ".";
const char PARENT[] = "..";

bool        path_is_relative(const char* path);
bool        path_is_absolute(const char* path);
bool        path_is_splitter(const char* path);
const char* path_skip_splitter(const char* path);
const char* path_rskip_splitter(const char* path);
bool        path_skip_current(const char** path);
bool        path_skip_parent(const char** path);
pathlen_t   name_length(const char* path);
int         name_compare(const char* name1, const char* name2);
pathlen_t   name_copy(const char* src, char* dest);
pathlen_t   name_normalize(const char* src, char* dest);

cause::pair<generic_ns*> create_initial_ns();

class path_parser
{
	NONCOPYABLE(path_parser);

public:
	using node_off = u16;

	struct node {
		fs_node* fsnode;
		char*    name;
	};

private:
	path_parser(generic_ns* fsns, node_off node_nr);
	~path_parser();
	static cause::pair<path_parser*> _create(
	    generic_ns* ns, node_off node_nr, u16 path_len);
	static cause::pair<path_parser*> _create(
	    generic_ns* ns, const char* cwd, const char* path);
	cause::pair<path_parser*> optimize();

public:
	static cause::pair<path_parser*> create(
	    generic_ns* ns, const char* cwd, const char* path);
	static cause::pair<path_parser*> create(
	    process* proc, const char* path);
	static void destroy(path_parser* x);

	node_off    get_node_nr() { return node_use_nr; }
	const char* get_path() const;
	generic_ns* get_ns() { return ns; }
	const node* get_node(node_off i);
	fs_node*    get_fsnode(node_off i);
	const char* get_name(node_off i);
	const node* get_edge_node();
	fs_node*    get_edge_fsnode();
	const char* get_edge_name();
	const node* get_edge_parent_node();
	fs_node*    get_edge_parent_fsnode();

private:
	cause::t follow_path(const char* path);
	char* get_path_buffer();
	void push_node(fs_node* fsnode, const char* name);
	bool pop_node();
	node* edge_node();

private:
	generic_ns* const ns;
	node_off const    node_buf_nr;
	node_off          node_use_nr;
	char*             path_end;
	node              nodes[];
};

}  // namespace fs


cause::t fs_mkdir(process* proc, const char* path);


#endif  // CORE_FS_HH_

