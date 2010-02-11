/**
 * @file    arch/x86_64/boot/bootldr/tr.S
 * @version 0.0.0.2
 * @author  Kato.T
 * @brief   Translate funcs.
 */
// (C) T.Kato 2010

.code16gcc

.section .text

.globl bios_block_copy, work_to_setup

#include "boot.h"

/**
 * int bios_block_copy(_u32 src, _u32 dest, _u32 words)
 * メモリブロックを1MiB越えのアドレスへ転送する。
 */
bios_block_copy:
	pushl %ecx
	pushl %esi
/*
	movw  $block_copy_gdt, %si
	movw  %ss, %ax
	movw  %ax, %es
	leaw  -48(%esp), %di
	movw  $24, %cx
	rep movsw
	leaw  -48(%esp), %si
	movl  12(%esp), %edx
	movl  %edx, 0x12(%si)
	movb  $0x93, 0x15(%si)
	movl  16(%esp), %edx
	movl  %edx, 0x1a(%si)
	movb  $0x93, 0x1d(%si)
*/
	pushw %cs
	popw  %es
	movw  $block_copy_gdt, %si
	movl  12(%esp), %eax
	movl  %eax, block_copy_gdt_src
	movb  $0x93, block_copy_gdt_src+3
	movl  16(%esp), %eax
	movl  %eax, block_copy_gdt_dest
	movb  $0x93, block_copy_gdt_dest+3

	movl  20(%esp), %ecx
	movl  $0x00008700, %eax
	int   $0x15
	jnc   1f
	orw   $0x8000, %ax
1:
//	movl  %eax, 0x1c(%esp)
	popl  %esi
	popl  %ecx
	ret
.align 8
block_copy_gdt:
	.word 0, 0, 0, 0, 0, 0, 0, 0
	.word 0xffff
block_copy_gdt_src:
	.word 0, 0
	.word 0
	.word 0xffff
block_copy_gdt_dest:
	.word 0, 0
	.word 0, 0, 0, 0, 0, 0, 0, 0, 0

/**
 * 作業アドレスからセットアップアドレスへ転送する。
 */
work_to_setup:
	pusha
	push  %ds
	movw  $BOOTLDR_WORK_SEG, %ax
	movw  %ax, %ds
	movw  $0, %si
	movw  $SETUP_SEG, %ax
	movw  %ax, %es
	movw  $SETUP_ADR, %di
	movw  $SETUP_MAXSIZE / 2, %cx
	rep movsw
	pop   %ds
	popa
	ret

/*
// acpi_get_memmap(_u32 handle, _u16 srcseg, _u16 srcmem)
// 最初は handle を 0 として呼び出す。
// 戻り値が 0 でなければ、戻り値を handle として再度呼び出すことで
// メモリマップの続きを取得できる。
acpi_get_memmap:
//	pusha
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushw %di

	movl  0x16(%esp), %ebx
	movw  0x2a(%esp), %ax
	movw  %ax, %es
	movw  0x2e(%esp), %ax
	movw  %ax, %di

	movl  $24, %ecx
	movl  $0x0000e820, %eax
	movl  $0x534d4150, %edx // 'SMAP'
	int   $0x15
	movl  %ebx, 0x1c(%esp)
//	movl  %eax, 0x1c(%esp)

	popw  %di
	popl  %edx
	popl  %ecx
	popl  %ebx
	popl  %eax
//	popa
	ret
*/

/*
// 文字列を表示する
// %si : 表示する文字列のポインタ
// %si, %ax, %bx は破壊される
display:
	lodsb
	testb %al, %al
	jz    end_display
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display
end_display:
	ret
*/
