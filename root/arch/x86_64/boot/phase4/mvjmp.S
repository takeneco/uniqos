/* FILE : arch/x86/boot/phase4/movejump.S
 * VER  : 0.0.2
 * LAST : 2009-06-08
 * (C)  : Kato.T 2009
 *
 * カーネルの32ビット部分を適当なアドレスへ移動し、
 * ジャンプする。
 *
 * phase3 から渡されるパラメータ：
 *  %ebx : phase4(movejump) の先頭アドレス
 *  %ecx : カーネルから phase3 を引いた長さ
 * phase4(setup32) へ渡すパラメータ：
 *  %ecx : phase4(setup32) からカーネルの終わりまでの長さ
 */

.code32

#include "boot.h"

MOVETO = 0x200000

.globl _start

.section .text

_start:
	movw  $2*8, %ax
	movw  %ax, %ds
	movw  %ax, %es

	// phase4 から phase4(movejump) を引いた長さ -> %ecx
	subl  $(_end - _start), %ecx
	pushl %ecx

	// phase4(setup32) の先頭アドレス -> %ebx
	addl  $(_end - _start), %ebx

	movl  %ebx, %esi
	movl  $MOVETO, %edi
	rep
	movsb
	popl  %ecx

	ljmpl $1*8, $MOVETO

// (%esi)を16進文字として(%edi), 2(%edi)に書く
// 1(%edi), 3(%edi)は$0x0fとしておく
// %ax, %ebxは壊れる 
print:
	movl  $print_base, %ebx
	addl  $PHASE4_ADDR, %ebx
	xorl  %eax, %eax
	movb  (%esi), %al
	shrb  $4, %al
	movb  (%ebx, %eax), %ah
	movb  %ah, (%edi)
	movb  $0xf, 1(%edi)
	xorl  %eax, %eax
	movb  (%esi), %al
	andb  $0x0f, %al
	movb  (%ebx, %eax), %ah
	movb  %ah, 2(%edi)
	movb  $0xf, 3(%edi)

	ret

print_base:
	.ascii "0123456789abcdefghijklmnopqrstuvwxyz"

_end:
