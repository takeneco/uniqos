// @file    arch/x86_64/boot/bootsect/bootsect.S
// @author  Kato Takeshi
// @brief   Boot sector.
//
// (C) 2010 Kato Takeshi

.code16

#include "boot.h"

.section .text

.global _start

_start:
	cld
	xorw  %ax, %ax
	movw  %ax, %ds
	movw  %ax, %es
	movw  %ax, %ss
	movw  $BOOTSECT_ADR, %sp

	// ブートドライブ番号(%dl)を保存する -> (BOOTSECT_ADR - 2)
	xorb  %dh, %dh
	pushw %dx;

	movw  $loading_msg, %si
	call  display

	/* すでに 0x7c00 以降に先頭の 1 セクタが読み込まれている。
	 * その続きの 17 セクタ(FD１周分)を 0x7e00 以降に読み込む。
	 */

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
	// ブートしたドライブから読み出す。
	//xorw  %dx, %dx
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
	// ロード済みセクタ数 -> (0x7c00 - 4)
	movw  $18, %ax
	pushw %ax

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
