/* FILE : arch/x86/boot/phase2/bios.S
 * VER  : 0.0.3
 * LAST : 2009-05-18
 * (C) T.Kato 2008-2009
 *
 * ブートローダのエントリーポイント。
 * カーネルを読み込み、処理をカーネルに移す。
 */

.code16gcc

.section .text

.global bios_block_copy, acpi_get_memmap

/**
 * int bios_block_copy(_u32 src, _u32 dest, _u32 words)
 * メモリブロックを1MiB越えのアドレスへ転送する。
 */
bios_block_copy:
	pusha
	movw  %ds, %ax
	movw  %ax, %es
	movw  $block_copy_gdt, %ax
	movw  %ax, %si
	movl  0x24(%esp), %edx
	movl  %edx, block_copy_gdt_src
	movw  $0x93, block_copy_gdt_src+3
	movl  0x28(%esp), %edx
	movl  %edx, block_copy_gdt_dest
	movw  $0x93, block_copy_gdt_dest+3
	movl  0x2c(%esp), %ecx
	movl  $0x00008700, %eax
	int   $0x15
	movl  %eax, 0x1c(%esp)
	popa
	ret
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

/*
loop:
	hlt
	movw $test, %si
	call display
	jmp   loop

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

test:
	.ascii "xxxx"
	.byte 0
*/
