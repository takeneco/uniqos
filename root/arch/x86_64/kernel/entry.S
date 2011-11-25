/// @file   entry.S
/// @brief  Kernel entry point.
//
// (C) 2010-2011 KATO Takeshi
//

.section .text.entry

.globl _start

_start:
	movq  $STACK_END, %rsp
	movq  %rax, %rdi
	call  kern_init
	call  kern_service
1:hlt
jmp 1b
