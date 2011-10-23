/// @brief multiboot entry point
//
// (C) 2011 KATO Takeshi
//

#define ASM_FILE
#include "multiboot2.h"

STACK_SIZE = 0x1000

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
	movl %eax, %edx

	movl $(stack_start + STACK_SIZE), %esp

	// clear stack zero.
	xorl %eax, %eax
	movl $stack_start, %edi
	movl $STACK_SIZE / 4, %ecx
	rep stosl

	pushl %ebx  // multiboot infomation
	pushl %edx  // multiboot magic
	call pre_load

	call load

	call post_load

	hlt
	jmp entry_

.size entry_, .-entry_

.globl stack_start
	.align 4
	.comm stack_start, STACK_SIZE

