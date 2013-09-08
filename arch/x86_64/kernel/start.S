/// @file   entry.S
/// @brief  Kernel entry point.
//
// (C) 2010-2013 KATO Takeshi
//

.section .text.entry

.globl _start

_start:
	movq  %rax, %rdi
	call  kern_init
1:hlt
jmp 1b
