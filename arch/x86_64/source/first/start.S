/// @file   first_process.S
/// @brief  Initial process entry point.
//
// (C) 2013-2014 KATO Takeshi
//

.section .start

.globl _start

_start:
	1:
	call main
	jmp 1b
