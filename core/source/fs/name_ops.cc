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

#include <core/new_ops.hh>
#include <util/string.hh>


namespace {

inline bool name_is_end(bool escape, const char *name)
{
	return *name == '\0' || (!escape && fs::path_is_splitter(name));
}

}  // namespace

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
    if (name_compare(*path, CURRENT) == 0) {
        *path += sizeof CURRENT - 1;
        return true;
    }

    return false;
}

bool path_skip_parent(const char** path)
{
    if (name_compare(*path, PARENT) == 0) {
        *path += sizeof PARENT - 1;
        return true;
    }

    return false;
}

pathlen_t name_length(const char* name)
{
	uint n;
	bool escape = false;

	for (n = 0; name[n]; ++n) {
		if (name_is_end(escape, name + n))
			break;

		escape = name[n] == ESCAPE;
	}

	return n;
}

/// @brief  Compare node name.
/// @return value < 0 if name1 less than name2,
///         value == 0 if name1 to match name2,
///         value > 0 if name1 greater than name2.
int name_compare(const char* name1, const char* name2)
{
	bool escape = false;

	for (;;) {
		if (*name1 != *name2) {
			bool name1_end = name_is_end(escape, name1);
			bool name2_end = name_is_end(escape, name2);
			if (name1_end && name2_end)
				return 0;
			if (name1_end)
				return -*name2;
			if (name2_end)
				return *name1;
			return *name1 - *name2;
		}
		if (name_is_end(escape, name1))
			return 0;

		escape = *name1 == ESCAPE;

		++name1;
		++name2;
	}
}

/// @brief   Copy node name.
/// @return  Copied node name byte nums without '\0'.
pathlen_t name_copy(const char* src, char* dest)
{
	uint n;
	bool escape = false;

	for (n = 0; ; ++n) {
		if (name_is_end(escape, src + n))
			break;

		dest[n] = src[n];

		escape = src[n] == ESCAPE;
	}

	dest[n] = '\0';

	return n;
}

/// @brief  Normalize node name.
/// @detail Remove meaningless escape code.
/// @return Normalized node name byte nums without '\0'.
pathlen_t name_normalize(const char* src, char* dest)
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

}  // namespace fs

