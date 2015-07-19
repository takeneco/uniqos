/// @file   clock_src.cc
/// @brief  Clock source operations.

//  Uniqos  --  Unique Operating System
//  (C) 2012 KATO Takeshi
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

#include <core/clock_src.hh>


void clock_source::operations::init()
{
	UpdateClock = 0;
	SetTimer = 0;
	ClockToNanosec = 0;
	NanosecToClock = 0;
}

