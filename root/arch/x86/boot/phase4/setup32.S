/* FILE : arch/x86/boot/phase4/setup32.S
 * VER  : 0.0.3
 * LAST : 2009-06-21
 * (C) Kato.T 2009
 *
 * 32ビットプロテクトモード移行後の
 * カーネル自身のセットアップ
 */

.code32

#include "boot.h"

.globl _start

.section .entry

_start:
	movw  $2*8, %ax
	movw  %ax, %ds
	movw  %ax, %es
	movw  %ax, %fs
	movw  %ax, %gs
	movw  %ax, %ss

	movl  $0x50000, %esp

	movl  $0x50000, %esi
	movl  $0xb8010, %edi
	call  print

//	incl  %esi
//	addl  $4, %edi
//	call  print

	incl  %esi
	addl  $4, %edi
	call  print

	incl  %esi
	addl  $4, %edi
	call  print

	call  setup_main

idle:
	hlt
	jmp idle

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
