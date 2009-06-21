/* FILE : arch/x86/kernel/start32.S
 * VER  : 0.0.1
 * LAST : 2009-06-21
 * (C) Kato.T 2009
 *
 * カーネル本体の先頭部分。
 */

.code32

.globl _start

.section .text

_start:
	hlt
	jmp   _start

