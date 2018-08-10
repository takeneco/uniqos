/// @file  core/syscall_wrap.hh
/// @brief System call interface wrapper.

//  Uniqos  --  Unique Operating System
//  (C) 2018 KATO Takeshi
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

#ifndef CORE_SYSCALL_WRAP_HH_
#define CORE_SYSCALL_WRAP_HH_

#include <core/basic.hh>


namespace uniqos {

template<
  cause::pair<ucpu> (*Func)()>
inline cause::pair<ucpu> syscall_wrap0(
    const ucpu[6])
{
    return Func();
}

template<
  class Arg0,
  cause::pair<ucpu> (*Func)(Arg0)>
inline cause::pair<ucpu> syscall_wrap1(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0]);
}

template<
  class Arg0, class Arg1,
  cause::pair<ucpu> (*Func)(Arg0, Arg1)>
inline cause::pair<ucpu> syscall_wrap2(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0],
        (Arg1)args[1]);
}

template<
  class Arg0, class Arg1, class Arg2,
  cause::pair<ucpu> (*Func)(Arg0, Arg1, Arg2)>
inline cause::pair<ucpu> syscall_wrap3(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0],
        (Arg1)args[1],
        (Arg2)args[2]);
}

template<
  class Arg0, class Arg1, class Arg2, class Arg3,
  cause::pair<ucpu> (*Func)(Arg0, Arg1, Arg2, Arg3)>
inline cause::pair<ucpu> syscall_wrap4(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0],
        (Arg1)args[1],
        (Arg2)args[2],
        (Arg3)args[3]);
}

template<
  class Arg0, class Arg1, class Arg2, class Arg3, class Arg4,
  cause::pair<ucpu> (*Func)(Arg0, Arg1, Arg2, Arg3, Arg4)>
inline cause::pair<ucpu> syscall_wrap5(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0],
        (Arg1)args[1],
        (Arg2)args[2],
        (Arg3)args[3],
        (Arg4)args[4]);
}

template<
  class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5,
  cause::pair<ucpu> (*Func)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5)>
inline cause::pair<ucpu> syscall_wrap6(
    const ucpu args[6])
{
    return Func(
        (Arg0)args[0],
        (Arg1)args[1],
        (Arg2)args[2],
        (Arg3)args[3],
        (Arg4)args[4],
        (Arg5)args[5]);
}

}  // namespace uniqos


#endif  // CORE_SYSCALL_HH_

