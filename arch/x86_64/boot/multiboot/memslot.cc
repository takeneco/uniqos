/// @file   memslot.cc
/// @brief  Prepare to load kernel for multiboot2.

//  UNIQOS  --  Unique Operating System
//  (C) 2014 KATO Takeshi
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

#include "misc.hh"

#include <bootinfo.hh>
#include <vga.hh>


text_vga vga_dev;

namespace {

const uptr BOOTHEAP_END = bootinfo::BOOTHEAP_END;

const struct {
	allocator::slot_index slot;
	uptr slot_head;
	uptr slot_tail;
} memory_slots[] = {
	{ SLOT_INDEX_CONVENTIONAL, 0x00000000,       0x000fffff   },
	{ SLOT_INDEX_BOOTHEAP,     0x00100000,       BOOTHEAP_END },
	{ SLOT_INDEX_NORMAL,       BOOTHEAP_END + 1, 0xffffffff   },
};

}  // namespace

void init_separator(separator* sep)
{
	for (u32 i = 0; i < sizeof memory_slots / sizeof memory_slots[0]; ++i)
	{
		sep->set_slot_range(
		    memory_slots[i].slot,
		    memory_slots[i].slot_head,
		    memory_slots[i].slot_tail);
	}
}

