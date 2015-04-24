/// @file  page_ctl.hh

//  Uniqos  --  Unique Operating System
//  (C) 2010-2015 KATO Takeshi
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

#ifndef ARCH_X86_64_PAGE_CTL_HH_
#define ARCH_X86_64_PAGE_CTL_HH_

#include <arch.hh>


namespace arch {

/// @brief Page control
class page_ctl
{
private:
	bool pse;     ///< page-size extensions for 32bit paging.
	bool pae;     ///< physical-address extension.
	bool pge;     ///< global-page support.
	bool pat;     ///< page-attribute table.
	bool pse36;   ///< 36bit page size extension.
	bool pcid;    ///< process-context identifiers.
	bool nx;      ///< execute disable.
	bool page1gb; ///< 1GByte pages.
	bool lm;      ///< IA-32e mode support.

	int padr_width;
	int vadr_width;

	void detect_paging_features();
};

}  // namespace arch


#endif  // include guard

