/* FILE : arch/x86/boot/phase1/bootsect.S
 * VER  : 0.0.5
 * LAST : 2009-05-24
 * (C) 2008-2009 Kato.T
 *
 * phase1 のブート処理（ブートセクタ）。
 */

.code16

#include "boot.h"

.section .text

.global _start

_start:
	xorw  %ax, %ax
	movw  %ax, %ss
	movw  $0x7c00, %sp
	movw  %ax, %ds
	movw  %ax, %es

	// ブートドライブ番号(%dl)を保存する -> (0x7c00 - 2)
	xorb  %dh, %dh
	pushw %dx;

	// すでに 0x7c00 以降に先頭の 1 セクタが読み込まれている。
	// その続きの 17 セクタを 0x7e00 (=PHASE2_ADDR) 以降に読み込む。

	movw  $load_msg, %si
	call  display

	// 失敗しても 8 回までは試す
	movw  $8, %di
load:
	// %ah : 読み込み = 0x02
	// %al : セクタ数 = 0x11 = 17
	// %bx : 読み込み先アドレス
	// %cx : 上位 10 ビットはシリンダ番号。下位 6 ビットはセクタ番号。
	//       セクタ番号は 1 から始まる
	// %dh : ヘッド
	// %dl : ドライブ
	movw  $0x0211, %ax
	movw  $PHASE2_ADDR, %bx
	movw  $0x0002, %cx
	xorw  %dx, %dx
	int   $0x13

	jnc   loadsuccess
	decw  %di
	jnz   load

	// 読み込みに失敗した場合は再起動する
	movw  $reboot_msg, %si
	call  display
	// キーを押すと再起動する。
	xorw  %ax, %ax
	int   $0x16
	int   $0x19

loadsuccess:
	// ロード済みセクタ数 -> (0x7c00 - 4)
	movw  $18, %ax
	pushw %ax
	jmp   PHASE2_ADDR

// 文字列を表示する
// %si : 表示する文字列のポインタ
// %si, %ax, %bx は破壊される
display:
	// lodsb :
	// %al = *(%si++) 
	lodsb
	testb %al, %al
	jz    1f
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display
1:
	ret

load_msg:
	.ascii "Loading ...\r\n"
	.byte 0
reboot_msg:
	.ascii "Load failed.\r\n"
	.ascii "Press any key and reboot...\r\n"
	.byte 0

