/// @file   build_info.cc
/// @brief  Dump build information.
//
/// Python の構文で出力する。

//  UNIQOS  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <output_buffer.hh>


namespace {

typedef output_buffer outbuf;

void _1(outbuf& ob)
{
	ob.str("1\n");
}

void _0(outbuf& ob)
{
	ob.str("0\n");
}


void dump_buildin_expect(outbuf& ob)
{
	ob.str("__builtin_expect = ");
#if defined(__GNUC__) || __has_builtin(__builtin_expect)
	_1(ob);
#else
	_0(ob);
#endif
}

void dump_clang(outbuf& ob)
{
#if defined(__clang__)
	ob.str("__clang_major__ = ").u(__clang_major__)
	  .str("\n__clang_minor__ = ").u(__clang_minor__)
	  .str("\n__clang_patchlevel__ = ").u(__clang_patchlevel__)
	  .str("\n__clang_version__ = \"")(__clang_version__)('"')();
#endif
}

void dump_GNUC(outbuf& ob)
{
#if defined(__GNUC__)
	ob.str("__GNUC__ = ").u(__GNUC__)
	  .str("\n__GNUC_MINOR__ = ").u(__GNUC_MINOR__)
	  .str("\n__GNUC_PATCHLEVEL__ = ").u(__GNUC_PATCHLEVEL__)();
#endif
}

}  // namespace

void dump_build_info(outbuf& ob)
{
	dump_clang(ob);
	dump_GNUC(ob);
	dump_buildin_expect(ob);
}

