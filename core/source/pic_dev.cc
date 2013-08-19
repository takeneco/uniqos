/// @file   pic_dev.cc
/// @brief  PIC(Programmable Interrupt Controller) control interface.

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
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

#include <pic_dev.hh>


void pic_device::operations::init()
{
	enable = pic_device::nofunc_pic_device_enable;
	disable = pic_device::nofunc_pic_device_disable;
	eoi = 0;
}


