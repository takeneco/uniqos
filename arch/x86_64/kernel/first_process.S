/// @file   first_process.S
/// @brief  Initial process.
//
// (C) 2013 KATO Takeshi
//

.section .text

.globl _start

_start:
	xorq %rax, %rax
	1:syscall
	incq %rax
	jmp 1b
