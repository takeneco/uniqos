// @file    arch/x86_64/kernel/entry.S
// @brief   Kernel entry point.
//
// (C) 2010 KATO Takeshi
//

.section .text.entry

.globl _start

_start:
	movq $STACK_END, %rsp
	hlt
	jmp _start
	call  kern_init
1:hlt
jmp 1b
