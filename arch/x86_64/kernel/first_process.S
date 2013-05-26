/// @file   pr.S
/// @brief  Initial process.
//
// (C) 2013 KATO Takeshi
//

.section .text

.globl _start

_start:
	1:hlt
	jmp 1b
