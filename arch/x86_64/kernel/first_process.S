/// @file   first_process.S
/// @brief  Initial process.
//
// (C) 2013 KATO Takeshi
//

.section .text

.globl _start

_start:
	1:
	movq $100, %rax
	syscall
	jmp 1b
