/// @brief multiboot header

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

#define ASM_FILE
#include <multiboot.h>


//
// multiboot header
//

.section .mbh

	.align MULTIBOOT_HEADER_ALIGN

mbh:
	// magic field
	.long MULTIBOOT_HEADER_MAGIC

	// flags field
#define MB_FLAGS (MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
	.long MB_FLAGS

	// checksum field
	.long -(MULTIBOOT_HEADER_MAGIC + MB_FLAGS)

	// header_addr
	.long 0

	// load_addr
	.long 0

	// load_end_addr
	.long 0

	// bss_end_addr
	.long 0

	// entry_addr
	.long 0

	// mode_type
	// 0: liner graphics mode, 1: EGA-standard text mode
	.long 1

	// width
	// 0: no preference
	.long 0

	// height
	// 0: no preference
	.long 0

	// depth
	// 0: no preference
	.long 0

mbh_end:
