/// @brief multiboot entry point
//
// (C) 2011 KATO Takeshi
//

#define ASM_FILE
#include "multiboot2.h"

//
// multiboot header
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
	.word 1, 0
	.long inforeq_end - inforeq
	.long 8
	.long 6
inforeq_end:

	// end of tags
	.word 0, 0
	.long 8
mbh_end:


//
// entry point
//

.section .entry

.code32

.globl entry_
entry_:
	pushl %ebx
	call pre

	hlt
	jmp entry_

.size entry_, .-entry_
