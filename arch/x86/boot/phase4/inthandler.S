/* FILE : inthandler.S
 * VER  : 0.0.3
 * LAST : 2009-04-16
 * (C)  : T.Kato 2009
 *
 * 割り込みハンドラ
 */

.globl _int21_handler // キーボード
.globl _int23_handler // COM2
.globl _int24_handler // COM1

.code32

.section .text

_int21_handler:
	pushw %es
	pushw %ds
	pushal
	movw  %ss, %ax
	movw  %ax, %ds
	movw  %ax, %es
	call  _int21h
	popal
	popw  %ds
	popw  %es
	iretl

_int23_handler:
	pushw %es
	pushw %ds
	pushal
	movw  %ss, %ax
	movw  %ax, %ds
	movw  %ax, %es
	call  _int23h
	popal
	popw  %ds
	popw  %es
	iretl

_int24_handler:
	pushw %es
	pushw %ds
	pushal
	movw  %ss, %ax
	movw  %ax, %ds
	movw  %ax, %es
	call  _int24h
	popal
	popw  %ds
	popw  %es
	iretl
