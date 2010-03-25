// @file    arch/x86_64/kernel/entry.S
// @author  Kato Takeshi
// @brief   Kernel entry point.
//
// (C) 2010 Kato Takeshi.

.section .text.entry

.globl _start

_start:
	call  KernInit
1:hlt
jmp 1b
