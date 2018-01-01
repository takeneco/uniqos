/// @file  fs/name_ops.cc
/// @brief Pathname operations.

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

#include <core/fs.hh>

#include <core/fs_ctl.hh>  // pathname_cn_t
#include <core/new_ops.hh>
#include <util/string.hh>


namespace fs {

bool path_is_absolute(const char* path)
{
	return *path == SPLITTER;
}

bool path_is_relative(const char* path)
{
	return *path != SPLITTER;
}

bool path_is_splitter(const char* name)
{
	return *name == SPLITTER;
}

/** @defgroup path_skip_splitter
 * @{
 * - path_skip_splitter() skips splitters forward. return value indicates
 * pointer to next character of splitter.
 * - path_rskip_splitter() skips splitters backwards. return value indicates
 * pointer of first charater of splitter.
 *
 * @code
 * char* a = "/xxx///yyy";
 *         // 0123456789
 *
 * path_skip_splitter(a + 4) returns (a + 7)
 * path_skip_splitter(a + 6) returns (a + 7)
 *
 * path_rskip_splitter(a + 7) returns (a + 4)
 * path_rskip_splitter(a + 5) returns (a + 4)
 * @endcode
 */
const char* path_skip_splitter(const char* path)
{
	while (*path == '/')
		++path;

	return path;
}

const char* path_rskip_splitter(const char* path)
{
	while (*(path - 1) == '/')
		--path;

	return path;
}

/// @}

bool path_skip_current(const char** path)
{
	if (name_compare(*path, CURRENT)) {
		*path += sizeof CURRENT - 1;
		return true;
	}

	return false;
}

bool path_skip_parent(const char** path)
{
	if (name_compare(*path, PARENT)) {
		*path += sizeof PARENT - 1;
		return true;
	}

	return false;
}

uptr name_length(const char* name)
{
	uptr r = 0;
	while (*name) {
		if (path_is_splitter(name))
			break;
		++name;
	}

	return r;
}

int name_compare(const char* name1, const char* name2)
{
	for (;;) {
		if (*name1 != *name2) {
			if ((*name1 == '\0' || *name1 == '/') &&
			    (*name2 == '\0' || *name2 == '/'))
				return 0;
			return *name1 - *name2;
		}
		if (*name1 == '\0' || *name1 == '/')
			return 0;

		++name1;
		++name2;
	}
}

/// @brief   Copy node name.
/// @return  Copied node name byte nums without '\0'.
uint name_copy(const char* src, char* dest)
{
	uint i;

	bool escape = false;

	for (i = 0; ; ++i) {
		if (!escape && (path_is_splitter(src + i) || src[i] == '\0'))
			break;

		dest[i] = src[i];

		escape = src[i] == ESCAPE;
	}

	dest[i] = '\0';

	return i;
}

/// @brief  Normalize node name.
/// @detail Remove meaningless escape code.
/// @return Normalized node name byte nums without '\0'.
uint name_normalize(const char* src, char* dest)
{
	uint i = 0;

	for (;;) {
		if (*src == ESCAPE) {
			switch (*(src + 1)) {
			case ESCAPE:
			case SPLITTER:
				dest[i++] = *src;
			// FALL THROUGH
			default:
				++src;
			}
		}
		if (!*src)
			break;

		dest[i++] = *src++;
	}

	dest[i] = '\0';

	return i;
}

bool nodename_is_last(const char* name)
{
	if (!*name)
		return false;

	for (;; ++name) {
		if (*name == '\0')
			return true;
		if (*name == '/')
			break;
	}

	name = path_skip_splitter(name);

	return *name == '\0';
}

const char* nodename_get_last(const char* path)
{
	const char* nodename = path_skip_splitter(path);
	for (;;) {
		const char* p = nodename;
		for (;; ++p) {
			if (path_is_splitter(p))
				break;
			if (*p == '\0')
				return nodename;
		}

		p = path_skip_splitter(p);
		if (*p == '\0')
			return nodename;
		else
			nodename = p;
	}
}

}  // namespace fs

