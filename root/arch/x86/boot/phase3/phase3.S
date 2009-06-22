/* FILE : arch/x86/boot/phase3/phase3.S
 * VER  : 0.0.4
 * LAST : 2009-05-26
 * (C) Kato.T 2009
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
 */

.code16

#include "boot.h"


.section .entry

.globl _start

_start:
	movw  %cs, %ax
	movw  %ax, %ds
	movw  %ax, %ss
	xorw  %ax, %ax
	movw  %ax, %sp

	jmp   start_setup

// メモリ書き込みで A20 ゲートの状態を確認する。
// ffff:7c10 に書いた内容が 0000:7c00 に反映されるか確認する。
// 呼び出し後、%ax と %bx が等しければ、A20 は開いている。
a20_test:
	xorw  %ax, %ax
	movw  %ax, %fs
	movw  $0xffff, %ax
	movw  %ax, %gs
	// ffff:7c10(%bx) に１を足し、0000:7c00 に書く。
	movw  %gs:0x7c10, %bx
	incw  %ax
	movw  %ax, %fs:0x7c00
	movw  %gs:0x7c10, %ax
	movw  %bx, %fs:0x7c00
	ret

// BIOS に A20 ゲートの状態を問い合わせる。
// A20 が閉じていれば %al が 0 に、開いていれば %al が 1 になる。
a20_status:
	movw  $0x2402, %ax
	int   $0x15
	ret

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
        lodsb
	testb %al, %al
	jz    1f
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display
1:	ret

// %es:%si から %fs:%di へメモリをコピーする。
// コピーするサイズは %cx の２倍
memcpy:
	movw  %es:(%si), %ax
	addw  $2, %si
	movw  %ax, %fs:(%di)
	addw  $2, %di
	decw  %cx
	jnz   memcpy
	ret

start_setup:

	pushl %ecx

	movw  $setupmsg, %si
	call  display

	// I/O 割り込みを禁止
	movb  $0xff, %al
	outb  %al, $0x21
	nop
	outb  %al, $0xa1
	cli

	// bios から A20 状態を取得する
	call  a20_status
	test  %al, %al
	jz    2f
	movw  $a20open_msg, %si
	jmp   3f
2:	movw  $a20close_msg, %si
3:	call  display

	// メモリアクセスによるA20の確認
	call  a20_test
	cmpw  %ax, %bx
	jne   4f
	movw  $a20open_msg, %si
	jmp   5f
4:	movw  $a20close_msg, %si
5:	call  display

	// A20 open
	call  a20_try_bios

	// bios から A20 状態を取得する
	call  a20_status
	test  %al, %al
	jz    6f
	movw  $a20open_msg, %si
	jmp   7f
6:	movw  $a20close_msg, %si
7:	call  display

	// メモリアクセスによる A20 の確認
	call  a20_test
	cmpw  %ax, %bx
	jne   8f
	movw  $a20open_msg, %si
	jmp   9f
8:	movw  $a20close_msg, %si
9:	call  display

	// 画面モード設定
	// AH = 0x00
	// AL = 0x03 : 16色テキスト 80x25
	//      0x12 : 640x480x4bits
	//      0x13 : 320x200x8bits
	//      0x6a : 800x600x4bits
	movw  $0x0003, %ax
//	int   $0x10

	calll setup16

	movw  $PH3_TO_PH4_PARAM_SEG, %ax
	movw  %ax, %es
	movl  $8, %es:PARAM_DISP_DEPTH    // ビット深度
//	movl  $320, %es:PARAM_DISP_WIDTH  // 画面の幅
	movl  $80, %es:PARAM_DISP_WIDTH
//	movl  $200, %es:PARAM_DISP_HEIGHT // 画面の高さ
	movl  $25, %es:PARAM_DISP_HEIGHT
//	movl  $0x000a0000, %es:PARAM_DISP_VRAM // VRAM
	movl  $0x0000b800, %es:PARAM_DISP_VRAM

	xorl  %eax, %eax
	movb  $0x02, %ah
	int   $0x16
	movl  %eax, %es:PARAM_KEYB_LEDS // キーボードの LED

	pushl $PARAM_MEMMAP
	pushl $PH3_TO_PH4_PARAM_SEG
	calll acpi_get_memmap
	addw  $8, %sp

	lgdtl boot_gdtr
	lidtl boot_idtr
	movl  %cr0, %eax
	orw   $0x0001, %ax
	movl  %eax, %cr0
	jmp   1f
1:

	// カーネルのサイズ
	popl  %ecx
	// カーネルから phase3 を引いたサイズ -> %ecx
	subl  $phase3_end, %ecx
	// phase4 の先頭アドレス -> %ebx
	movl  $PHASE4_ADDR + phase3_end, %ebx
	ljmpl $1*8, $PHASE4_ADDR + phase3_end

	.align 16
boot_gdt:
	// 3.4.3(p.3-12)
	.byte 0, 0, 0, 0, 0, 0, 0, 0
	// base  0x00000000
	// limit 0xffffff
	// [1]コードセグメント
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00
	// [2]データセグメント
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00

	.align 4
boot_gdtr:
	.short 8*3 - 1
	.long boot_gdt + 0x30000
boot_idtr:
	.short 0
	.long 0

setupmsg:
	.ascii "setup executing...\r\n"
	.byte 0
a20open_msg:
	.ascii "A20 gate opened.\r\n"
	.byte 0
a20close_msg:
	.ascii "A20 gate closed.\r\n"
	.byte 0

phase4:
