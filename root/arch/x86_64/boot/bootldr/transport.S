/// @file  tr.S
/// @brief memory transport funcs.
//
// (C) 2010 KATO Takeshi
//

.code16gcc

.section .text

.globl bios_block_copy, work_to_setup

#include "boot.h"


/**
 * int bios_block_copy(u32 src, u32 dest, u32 words)
 * メモリブロックを1MiB越えのアドレスへ転送する。
 */
bios_block_copy:
	pushl %ecx
	pushl %esi

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
