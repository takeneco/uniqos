/// @brief multiboot2 header

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
#include <multiboot2.h>


//
// multiboot2 header
//

.section .mbh

	.align MULTIBOOT_HEADER_ALIGN

	// magic field
mbh:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long mbh_end - mbh
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (mbh_end - mbh))

	// infomation request
inforeq:
	.word MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST, 0
	.long inforeq_end - inforeq
	.long MULTIBOOT_TAG_TYPE_MMAP
	.long MULTIBOOT_TAG_TYPE_FRAMEBUFFER
inforeq_end:

	// end of tags
	.word MULTIBOOT_HEADER_TAG_END, 0
	.long 8
mbh_end:
