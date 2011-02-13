/// @file  bootsect.S
/// @brief Boot sector.
//
// (C) 2010 KATO Takeshi
//

.code16

#include "boot.h"

.section .text

.global _start
_start:
	cld
	xorw  %ax, %ax
	movw  %ax, %ds
	movw  $BOOTLDR_SEG, %ax
	movw  %ax, %es
	movw  $BOOT_STACK_SEG, %ax
	movw  %ax, %ss
	movw  $BOOT_STACK_ADR, %sp

	// store boot drive number(%dl) -> (BOOT_STACK_ADR - 2)
	xorb  %dh, %dh
	pushw %dx

	movw  $loading_msg, %si
	call  display

// first 1sect(bootsect) was already loaded
// to 0x7c00(=INIT_BOOTSECT_ADR) by BIOS.
// copy bootsect to 0x1000(=BOOTLDR_SEG):0000(=BOOTLDR_ADR-1sect).

	movw  $0x200 / 2, %cx
	movw  $INIT_BOOTSECT_ADR, %si
	movw  $BOOTSECT_ADR, %di
1:	movsw   // *(es:di++) <- *(ds:si++)
	loop  1b

// and load next 17 sectors (= 1round of floppy) to BOOTLDR_SEG:BOOTLDR_ADR.

	// 失敗しても 255 回までは試す
	movw  $255, %di
load:
	// %ah     : 読み込み = 0x02
	// %al     : セクタ数 = 0x11 = 17
	// %es:%bx : 読み込み先アドレス
	// %cx     : 上位 10 ビットはシリンダ番号。下位 6 ビットはセクタ番号。
	//           セクタ番号は 1 から始まる
	// %dh     : ヘッド
	// %dl     : ドライブ
	movw  $BOOTLDR_ADR, %bx
	movw  $0x0211, %ax
	movw  $0x0002, %cx
	// ブートしたドライブから読み出すため、dl はそのまま。
	//xorw  %dl, %dl
	int   $0x13

	jnc   load_success
	decw  %di
	jnz   load

	// 読み込みに失敗した場合は再起動する
	movw  $reboot_msg, %si
	call  display
	// キーを押すと再起動する。
	xorw  %ax, %ax
	int   $0x16
	int   $0x19

load_success:
	// ロード済みセクタ数 -> (BOOT_STACK_ADR - 4)
	pushw $18

	ljmp  $BOOTLDR_SEG, $BOOTLDR_ADR

// 文字列を表示する
// %si : 表示する文字列のポインタ
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
1:
	ret

loading_msg:
	.ascii "Loading ...\r\n"
	.byte 0
reboot_msg:
	.ascii "Load failed.\r\n"
	.ascii "Press any key and reboot...\r\n"
	.byte 0
