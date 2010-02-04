/**
 * @file    arch/x86/kernel/start32.S
 * @version 0.0.2
 * @date    2009-08-03
 * @author  Kato.T
 * @brief   カーネル本体の先頭部分。
 */
// (C) Kato.T 2009

.code32

.globl _start

.section .text.entry

_start:
	call  init
idle:
	hlt
	jmp   idle

