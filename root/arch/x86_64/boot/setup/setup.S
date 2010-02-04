/**
 * @file    arch/x86_64/boot/setup/setup.S
 * @version 0.0.0.1
 * @author  Kato.T
 *
 * 16ビットリアルモードの間に実行する、
 * カーネル自身のセットアップ。
 * 起動アドレスを変える場合は、lgdtrのポインタを変える必要がある。
 *
 * phase2 から渡されるパラメータ：
 *  %ecx : カーネルのサイズ
 * phase4 へ渡すパラメータ：
 *  %ebx : phase4 の先頭アドレス
 *  %ecx : カーネルから phase3 を引いたサイズ
 *  メモリ(PH3_4_PARAM_SEG) : BIOS/ACPI から取得したパラメータ
 */
// (C) Kato.T 2009-2010

#include "boot.h"

.section .entry

.globl _start
.globl a20isopen, a20isclose

_start:
.code16
	movw  %cs, %ax
	movw  %ax, %ds
	movw  $SETUP_STACK_ADR >> 4, %ax
	movw  %ax, %ss
	xorw  %ax, %ax
	movw  %ax, %sp

	jmp   start_setup

// BIOS 経由で A20 ゲートを開く。
a20_try_bios:
	pushfl
	movw  $0x2401, %ax
	int   $0x15
	popfl
	ret

// 文字列を表示する
// %si : 表示するヌル終端文字列のポインタ
// %si, %ax, %bx は破壊される
display:
	// lodsb : %al = *(%ds:%si++) 
        lodsb
	testb %al, %al
	jz    1f
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display
1:	ret

start_setup:
	pushl %ecx

	movw  $setup_msg, %si
	call  display

	// Protect I/O interrupt
	movb  $0xff, %al
	outb  %al, $0x21
	nop
	outb  %al, $0xa1
	cli

	call  a20_test_bios
	movw  %ax, %si
	call  display

	call  a20_test_mem
	movw  %ax, %si
	call  display

	call  a20_open_bios  // A20 open

	call  a20_test_bios
	movw  %ax, %si
	call  display

	call  a20_test_mem
	movw  %ax, %si
	call  display

	// 画面モード設定
	// AH = 0x00
	// AL = 0x03 : 16色テキスト 80x25
	//      0x12 : 640x480x4bits
	//      0x13 : 320x200x8bits
	//      0x6a : 800x600x4bits
//	movw  $0x0003, %ax
//	int   $0x10

//	calll setup16

	movw  $SETUP_DATA_SEG, %ax
	movw  %ax, %es
	movl  $8, %es:SETUP_DISP_DEPTH    // ビット深度(グラフィックモード)
//	movl  $320, %es:SETUP_DISP_WIDTH  // 画面の幅
	movl  $80, %es:SETUP_DISP_WIDTH
//	movl  $200, %es:SETUP_DISP_HEIGHT // 画面の高さ
	movl  $25, %es:SETUP_DISP_HEIGHT
//	movl  $0x000a0000, %es:SETUP_DISP_VRAM // VRAM
	movl  $0x000b8000, %es:SETUP_DISP_VRAM

	xorl  %eax, %eax
	movb  $0x02, %ah
	int   $0x16
	movl  %eax, %es:SETUP_KEYB_LEDS // キーボードの LED

	// テキストモードのカーソル位置を取得する。
	movb  $0x03, %ah
	xorb  %bh, %bh
	int   $0x10
	xorl  %eax, %eax
	movb  %dl, %al
	movl  %eax, %es:SETUP_DISP_CURCOL
	movb  %dh, %al
	movl  %eax, %es:SETUP_DISP_CURROW

/*
	pushl $SETUP_MEMMAP
	pushl $SETUP_DATA_SEG
	calll acpi_get_memmap
	addw  $8, %sp
	// %eax != 0xffffffff でエラーチェックが必要
	movl  %eax, %es:PH3_4_MEMMAP_COUNT
*/
	// カーネルのサイズ
	popl  %ecx
	// カーネルから phase3 を引いたサイズ -> %ecx
	//subl  $phase3_end, %ecx
	// phase4 の先頭アドレス -> %ebx
	//movl  $PHASE4_ADDR + phase3_end, %ebx

	lgdtl boot_gdtr32
	lidtl boot_idtr32
	movl  %cr0, %eax
	orl   $0x00000001, %eax
	movl  %eax, %cr0

	//ljmpl $2*8, $PHASE4_ADDR + start32
	ljmpl $2*8, $start32

//16bitではPHASE3_ADDRのセグメントにいるが、
//32bitではセグメントがリセットされる。
	.align 16
boot_gdt32:
	// 3.4.3(p.3-12)
	.byte 0, 0, 0, 0, 0, 0, 0, 0
	// base  0x00000000
	// limit 0xffffff
	// [1]64bitコードセグメント
	.word 0xffff, 0x0000, 0x9a00, 0x00af
	// [2]32bitコードセグメント
	//    base : 0x10000
	.word 0xffff, 0x0000, 0x9a01, 0x00cf
	// [3]32bitデータセグメント
	//    base : 0x10000
	.word 0xffff, 0x0000, 0x9201, 0x00cf

	.align 4
boot_gdtr32:
	.short 8*4 - 1
	.long boot_gdt32 + 0x10000
boot_idtr32:
	.short 0
	.long 0

setup_msg:
	.ascii "setup executing...\r\n"
	.byte 0
a20isopen:
	.ascii "A20 gate opened.\r\n"
	.byte 0
a20isclose:
	.ascii "A20 gate closed.\r\n"
	.byte 0


start32:
.code32
	movw  $3*8, %ax
	movw  %ax, %ds
	movw  %ax, %es
	movw  %ax, %fs
	movw  %ax, %gs
	movw  %ax, %ss
	movl  $SETUP_STACK_ADR, %esp

	// Enable PAE
	movl  $0x00000020, %eax
	movl  %eax, %cr4

1:hlt
jmp 1b
